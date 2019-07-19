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
Param = namedtuple('Param', ['name', 'type'])
Func = namedtuple('Func', ['realname', 'name', 'ret_type', 'args'])

def _iter_read(iterator):
    return lambda x=-1: next(iterator).encode('utf-8')

def c_format(vartype):
    return ' '.join(vartype)

TYPES = {
    'HexColor': 'hex_color',
    'i32': 'int',
    'u32': 'int',
    'int': 'int',
    'int32_t': 'int',
    'uint32_t': 'int',
    'float': 'float',
    'f32': 'float',
    ('char', '*'): 'string',
}

def enumtype(vartype):
    if len(vartype) == 1:
        vartype = vartype[0]
    return TYPES[vartype].upper()

def parse_next_decl(isrc, **hints):
    tok_list = []
    p = tokenize(_iter_read(isrc))  # TEMP: should use actual C/++ tokenizer
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
            # is a function
            pass
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

    # Set up variable linkage
    for decl in variables.values():
        print('extern', c_format(decl.type), decl.realname + ';', file=out)

    print("const ConsoleVar console_vars[] = {", file=out)
    for decl in variables.values():
        print(f"\t{{ &{decl.realname}, \"{decl.name}\", T_{enumtype(decl.type)} }},", file=out)
    print("};", file=out)
    print(f"const size_t n_console_vars = {len(variables)};", file=out)

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
