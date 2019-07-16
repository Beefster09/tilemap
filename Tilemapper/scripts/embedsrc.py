
import argparse
import itertools
import glob
import os
import sys

def make_id(filename, prefix, suffix):
    return prefix + os.path.basename(infile).upper().replace('.', '_') + suffix

def output_c_const(infile, var, file=sys.stdout):
    print("const char*", var, '= ( ""', file=file)
    with open(infile) as f:
        for line in f:
            print('"', str(line.encode('unicode-escape'), 'utf-8').replace('"', '\"'), '"', sep='', file=file)
    print(");", file=file)

if __name__ == '__main__':
    parser = argparse.ArgumentParser()

    parser.add_argument('files', nargs="+")
    parser.add_argument('-o', '--outfile')
    parser.add_argument('-p', '--prefix', default='')
    parser.add_argument('-s', '--suffix', default='')
    parser.add_argument('--lazy', action='store_true', help="Update the outfile only if it is older than all its sources.")

    args = parser.parse_args()

    if any('*' in filename for filename in args.files):  # fix for windows not handling globs correctly
        args.files = [*itertools.chain.from_iterable(
            glob.iglob(pattern) for pattern in args.files
        )]

    print("Embedding", '; '.join(args.files), 'into', args.outfile)

    if args.outfile:
        if args.lazy and os.path.isfile(args.outfile):
            outstat = os.stat(args.outfile)
            if all(os.stat(f).st_mtime < outstat.st_mtime for f in args.files):
                print("Outfile is newer than all sources. Nothing to do.", file=sys.stderr)
                sys.exit(0)
        out = open(args.outfile, 'w')
    else:
        out = sys.stdout

    for infile in args.files:
        var = make_id(infile, args.prefix, args.suffix)
        print(file=out)
        print('const char*', var + '__SRC', '= "%s";' % os.path.basename(infile), file=out)
        output_c_const(infile, var, file=out)

