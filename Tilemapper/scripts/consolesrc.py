import argparse
import itertools
import glob
import os
import re
import sys
from tokenize import *
from collections import namedtuple

class NameCollision(Exception):
    pass

class IncompatibleDeclaration(Exception):
    def __str__(self):
        return f"Incompatible Declaration: '{' '.join(self.args[0])}'"

Var = namedtuple('Var', ['realname', 'name', 'type'])
Param = namedtuple('Param', ['name', 'type', 'default'])
Func = namedtuple('Func', ['realname', 'name', 'ret_type', 'params'])

TYPES = {
    'HexColor': 'hex_color',
    'i32': 'int',
    'int32_t': 'int',
    'u32': 'uint',
    'uint32_t': 'uint',
    'f32': 'float',
    ('char', '*'): 'string',
}

BLOCK_COMMENT = re.compile(r'/\*.*?\*/')
LINE_COMMENT = re.compile(r'//.*')

def _iter_read(iterator):
    def _readline(n_bytes=-1):
        line = next(iterator)
        # strip comments
        line = LINE_COMMENT.sub('', line)
        line = BLOCK_COMMENT.sub('', line)
        line = line.replace('::', '.')
        return line.encode('utf-8')
    return _readline

def c_format(vartype):
    return ' '.join(vartype)

def enumtype(vartype):
    stripped = tuple([
        token for token in vartype
        if token not in ('&', 'const')
    ])
    if len(vartype) == 1:
        vartype = vartype[0]
    return TYPES.get(vartype, vartype).replace(' ', '_').upper()

def parse_param(p):
    tok_list = []
    name = None
    ptype = None
    default = None
    for token in p:
        # TODO? nesting parentheses
        if token.string == '=':
            name = tok_list[-1]
            ptype = tuple(tok_list[:-1])
            tok_list = []
        elif token.string in ',)':
            if name:
                default = ' '.join(tok_list)
            elif tok_list:
                name = tok_list[-1]
                ptype = tuple(tok_list[:-1])
            else:
                return None, token.string != ')'
            return Param(name, ptype, default), token.string != ')'
        elif token.string == '.': #actually ::
            tok_list.append('::')
        else:
            tok_list.append(token.string)

def parse_next_decl(isrc, **hints):
    tok_list = []
    p = tokenize(_iter_read(isrc))
    for token in p:
        if token.type == ENCODING:
            continue
        #TODO: reject incompatible declarations
        if token.string in '=;':
            # is a variable
            if 'const' in tok_list or 'static' in tok_list:
                raise IncompatibleDeclaration(tok_list)
            realname = tok_list[-1]
            return Var(realname, hints.get('name', realname), tuple(tok_list[:-1]))

        elif token.string == '(':
            if 'static' in tok_list:
                raise IncompatibleDeclaration(tok_list)
            realname = tok_list[-1]
            ret_type = tuple(tok_list[:-1])
            params = []
            while True:
                param, is_more = parse_param(p)
                if param:
                    params.append(param)
                if not is_more:
                    return Func(
                        realname,
                        hints.get('name', realname),
                        ret_type,
                        params
                    )

        elif '.' in tok_list:
            raise IncompatibleDeclaration(tok_list)

        else:
            tok_list.append(token.string)

CONSOLE_TAG = re.compile(r'///?\s*@console\b')
def scan_source(filename):
    with open(filename) as src:
        isrc = iter(src)
        for line in isrc:
            line = line.strip()
            if CONSOLE_TAG.match(line):
                _, hints_raw = line.split('@console', 1)
                hints = dict(
                    h.split('=', 1) if '=' in h else (h, True)
                    for h in hints_raw.split()
                )
                try:
                    yield parse_next_decl(isrc, **hints)
                except IncompatibleDeclaration as e:
                    print(e, 'in', repr(filename), file=sys.stderr)

def output_console_bindings(decls, out=sys.stdout):
    variables = {}
    realvars = set()
    functions = {}
    # Check for naming conflicts
    for decl in decls:
        if isinstance(decl, Var):
            if decl.name in variables or decl.realname in realvars:
                raise NameCollision(decl.name)
            variables[decl.name] = decl
            realvars.add(decl.realname)
        elif isinstance(decl, Func):
            if decl.name in functions:
                raise NameCollision(decl.name)
            functions[decl.name] = decl

    # Set up variable linkage
    for decl in variables.values():
        print('extern', c_format(decl.type), decl.realname + ';', file=out)

    print("const ConsoleVar console_vars[] = {", file=out)
    for decl in variables.values():
        print(f"\t{{ &{decl.realname}, \"{decl.name}\", T_{enumtype(decl.type)} }},", file=out)
    print("};", file=out)
    print(f"const size_t n_console_vars = {len(variables)};", file=out)
    print(file=out)

    # declare function types so that we can call them
    for decl in functions.values():
        print(
            f"{c_format(decl.ret_type)} {decl.realname}"
            f"({', '.join(c_format(param.type) for param in decl.params)});",
            file=out
        )
    print(file=out)

    # create wrapper functions
    for decl in functions.values():
        print(f"static CommandStatus {decl.name}__wrapper(int __n_args, Any* __args, void* __ret_ptr) {{", file=out)
        print(f"\tif (__n_args < {len(decl.params)}) return CMD_NOT_ENOUGH_ARGS;", file=out)
        for i, param in enumerate(decl.params):
            print(f"\t{c_format(param.type)} {param.name} = __args[{i}].v_{enumtype(param.type).lower()};", file=out)
        call = f"{decl.realname}({', '.join(param.name for param in decl.params)})"
        if decl.ret_type == ('void',):
            print(f"\t{call};", file=out)
        else:
            print(f"\t{c_format(param.type)} __result = {call};", file=out)
            print(f"\t*(({c_format(param.type)}*)__ret_ptr) = __result;", file=out)
        print("\treturn CMD_OK;", file=out)
        print("}", file=out)
        print(file=out)

    # create default arguments
    for decl in functions.values():
        for i, param in enumerate(decl.params):
            if param.default is not None:
                print(f"{c_format(param.type)} const {decl.name}__arg{i}_default = {param.default};", file=out)

    print("const ConsoleFunc console_funcs[] = {", file=out)
    for decl in functions.values():
        print("\t{", file=out)
        print(f"\t\t&{decl.name}__wrapper, \"{decl.name}\", ", file=out)
        print(f"\t\t{len(decl.params)}, {{", file=out)
        for param in decl.params:
            default = (
                f'(void*)&{decl.name}__arg{i}_default'
                if param.default is not None else 'nullptr'
            )
            print(f"\t\t\t{{ T_{enumtype(param.type)}, \"{param.name}\", {default} }},", file=out)
        print("\t\t},", file=out)
        print(f"\t\tT_{enumtype(decl.ret_type)}", file=out)
        print("\t},", file=out)
    print("};", file=out)
    print(f"const size_t n_console_funcs = {len(functions)};", file=out)

if __name__ == '__main__':
    parser = argparse.ArgumentParser()

    parser.add_argument('files', nargs="+")
    parser.add_argument('-o', '--outfile')
    parser.add_argument(
        '--lazy',
        action='store_true',
        help="Update the outfile only if it is older than all its sources."
        # TODO: make this smarter about writing to the output file
    )

    args = parser.parse_args()

    if any('*' in filename for filename in args.files):  # fix for windows not handling globs correctly
        args.files = [
            file for file in itertools.chain.from_iterable(
                glob.iglob(pattern) for pattern in args.files
            )
            if os.path.isfile(file)
        ]

    print("Scanning", '; '.join(args.files), 'for console commands.')

    if args.outfile:
        if args.lazy and os.path.isfile(args.outfile):
            outstat = os.stat(args.outfile)
            if all(os.stat(f).st_mtime < outstat.st_mtime for f in args.files):
                print("Outfile is newer than all sources. Nothing to do.", file=sys.stderr)
                sys.exit(0)
        out = open(args.outfile, 'w')
    else:
        out = sys.stdout

    output_console_bindings(
        itertools.chain.from_iterable(
            scan_source(filename)
            for filename in args.files
        ),
        out=out
    )
