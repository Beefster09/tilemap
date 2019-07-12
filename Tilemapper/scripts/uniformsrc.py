
import argparse
import itertools
import re
import os
import sys

UNIFORM_PATTERN = re.compile(r'uniform\s+\w+\s+(\w+)\s*(?:=.*)?;')

def scan_shader(infile):
    with open(infile) as f:
        for line in f:
            m = UNIFORM_PATTERN.match(line)
            if m:
                yield m.group(1)

if __name__ == '__main__':
    parser = argparse.ArgumentParser()

    parser.add_argument('files', nargs="+")
    parser.add_argument('-o', '--outfile')
    parser.add_argument('-m', '--macro', default='__SLOT')
    parser.add_argument('-x', '--exclude', nargs='+', default=['camera', 'transform'])
    parser.add_argument('--lazy', action='store_true', help="Update the outfile only if it is older than all its sources.")

    args = parser.parse_args()

    if args.outfile:
        if args.lazy and os.path.isfile(args.outfile):
            outstat = os.stat(args.outfile)
            if all(os.stat(f).st_mtime < outstat.st_mtime for f in args.files):
                print("Outfile is newer than all sources. Nothing to do.", file=sys.stderr)
                sys.exit(0)
        out = open(args.outfile, 'w')
    else:
        out = sys.stdout

    uniforms = sorted(set(itertools.chain.from_iterable(
        scan_shader(infile)
        for infile in args.files
    )) - set(args.exclude))
    for uniform in uniforms:
        print(f"{args.macro}({uniform});", file=out)

