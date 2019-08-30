import argparse
import itertools
import shlex
import os
import sys

PIXEL_ALIAS = {'X': 255, 'o': 127}
OPENGL_MIN_TEX_SIZE = 64

def split_unquoted(line, max_split):
    lexer = shlex.shlex(line, posix=True) # TEMP: this handles some cases weirdly
    lexer.whitespace = ':'
    lexer.whitespace_split = True
    lexer.punctuation_chars = ''
    result = []
    while True:
        token = lexer.get_token()
        if token is None:
            return [*result[:max_split], ':'.join(result[max_split:])]
        else:
            result.append(token)

def read_font(filename):
    data = {
        'placeholder': (0, 0, 0, 0, 0)
    }
    mode = None
    bitmap = []
    glyphs = {}
    arrays = {}
    kerning = {}

    eval_ctx = {'__builtins__': None}

    def _char(ch):
        ch = ch.strip()
        if len(ch) == 1:
            return ord(ch)
        elif ch.startswith('0x'):
            return int(ch, 16)
        elif ch.startswith('#'):
            return int(ch[1:], 16)
        elif ch.startswith('0'):
            return int(ch, 8)
        elif ch.isdigit():
            return int(ch)
        else:
            raise ValueError(f"Can't interpret character: {ch!r}")

    with open(filename) as fp:
        f = iter(fp)
        for l in f:
            line = l.strip()
            if line.startswith(('#', '//')) or not line: # is a comment/empty
                continue

            if mode is None:
                attr, *args = line.split()
                attr = attr.lower()
                if attr == 'bitmap':
                    if args[0] == 'inline':
                        mode = 'bitmap'
                elif attr == 'array':
                    mode = 'array', args[0]
                    arrays[args[0]] = []
                elif attr in ('glyphs', 'kerning'):
                    mode = attr
                elif attr in ('placeholder', 'cursor'):
                    data[attr] = eval(' '.join(args), eval_ctx)
                else:
                    data[attr] = ' '.join(args)

            elif mode == 'bitmap':
                if line.lower() == 'end':
                    # de-jaggify and pad to OpenGL specs
                    longest = max(max(map(len, bitmap)), OPENGL_MIN_TEX_SIZE)
                    for row in bitmap:
                        if len(row) < longest:
                            row += [0] * (longest - len(row))
                    while len(bitmap) < OPENGL_MIN_TEX_SIZE:
                        bitmap.append([0] * longest)
                    mode = None
                else:
                    bitmap.append([
                        int(pixel) if pixel.isdigit() else PIXEL_ALIAS.get(pixel, 0)
                        for pixel in line.split()
                    ])

            elif mode == 'glyphs':
                if line.lower() == 'end':
                    mode = None
                    continue
                pattern, value = split_unquoted(line, 1)
                pattern = pattern.strip()
                if pattern != '-' and '-' in pattern:
                    lo, hi = map(_char, pattern.split('-'))
                    for i, ch in enumerate(range(lo, hi+1)):
                        c = chr(ch)
                        try:
                            result = eval(value, eval_ctx, {**arrays, 'i': i, 'c': c})
                            glyphs[c] = [int(x) for x in result]
                        except Exception:
                            print(i, c, value)
                            raise
                else:
                    ch = _char(pattern)
                    glyphs[chr(ch)] = [
                        int(x)
                        for x in eval(value, eval_ctx)
                    ]

            elif mode == 'kerning':
                if line.lower() == 'end':
                    mode = None
                    continue
                first, second, offset = split_unquoted(line, 2)
                first = first.strip()
                second = second.strip()

                for a in first:
                    for b in second:
                        kerning[ord(a), ord(b)] = int(offset)

            elif mode[0] == 'array':
                if line.lower() == 'end':
                    mode = None
                    continue

                arr = mode[1]
                arrays[arr] += eval(f'[{line}]', eval_ctx)

    placeholder = data['placeholder']
    for ascii_glyph in range(0x21, 0x7f):
        glyphs.setdefault(chr(ascii_glyph), placeholder)
    return {
        **data,
        'bitmap': bitmap,
        'glyphs': glyphs,
        'kerning': kerning,
    }

def output_c_header(font_data):
    name = font_data['name']
    glyphs = font_data['glyphs']

    print(f"constexpr int {name}__bitmap_width = {len(font_data['bitmap'][0])};", file=out)
    print(f"constexpr int {name}__bitmap_height = {len(font_data['bitmap'])};", file=out)
    print(f'const unsigned char {name}__bitmap[] = {{', file=out)
    for row in font_data['bitmap']:
        print(*row, sep=', ', end=',\n', file=out)
    print('};\n', file=out)

    print(f"constexpr int {name}__n_kern_pairs = {len(font_data['kerning'])};", file=out)
    print(f'const KernPair {name}__kerning[] = {{', file=out)
    for (a, b), offset in sorted(font_data['kerning'].items()):
        if offset:
            print(f"{{ {a}, {b}, {offset} }},", file=out)
    print('};\n', file=out)

    print(f'Font {name} = {{', file=out)
    print( '  nullptr,', file=out)
    print( '  nullptr,', file=out)
    print( '  -1,', file=out)
    print( '  {', file=out)
    for ascii_glyph in range(0x21, 0x7f):
        c = chr(ascii_glyph)
        print(f"  /* {c} */ {{ {', '.join(map(str, glyphs.get(c, (0,) * 5)))} }},", file=out)
    print( '  },', file=out)
    print(f"  {{ {', '.join(map(str, font_data['cursor']))} }},", file=out)
    print(f"  {font_data['space']},", file=out)
    print(f"  {font_data['height']},", file=out)
    # print(f"  {name}__kerning, {len(font_data['kerning'])}", file=out)
    print( '};', file=out)

if __name__ == '__main__':
    parser = argparse.ArgumentParser()

    parser.add_argument('file')
    parser.add_argument('-o', '--outfile')
    parser.add_argument('-f', '--format', default='c_header')
    parser.add_argument('--lazy', action='store_true', help="Update the outfile only if it is older than all its sources.")

    args = parser.parse_args()

    if args.outfile:
        print("Generating font", args.file, "into", args.outfile)
        if args.lazy and os.path.isfile(args.outfile):
            outstat = os.stat(args.outfile)
            if os.stat(args.file).st_mtime < outstat.st_mtime:
                print("Outfile is newer than its source. Nothing to do.", file=sys.stderr)
                sys.exit(0)
        out = open(args.outfile, 'w')
    else:
        out = sys.stdout

    font_data = read_font(args.file)
    if args.format == 'c_header':
        try:
            output_c_header(font_data)
        except Exception:
            if out is not sys.stdout:
                out.close()
                os.remove(args.outfile)
            raise
