import os
import sys
import subprocess

def main():
    set_size = 35
    chunk_sizes = [10, 14, 17, 20, 24, 26, 27, 28, 29, 30]
    max_num = 10000

    null = open(os.devnull, 'w')
    for chunk_size in chunk_sizes:
        dirname = 'data/%d' % chunk_size
        if not os.path.exists(dirname):
            os.makedirs(dirname)
        for i in xrange(min(1<<(set_size - chunk_size), max_num)):
            cmd_dd = ['dd',
                'if=/dev/urandom',
                'of=%s/%d.bin' % (dirname, i),
                'bs=%d' % (1<<chunk_size),
                'count=1',
                'iflag=fullblock'
            ]
            subprocess.check_call(cmd_dd, stdin=null, stdout=null, stderr=null)
            sys.stdout.write('\rchunksize 1<<%d: done %d' % (chunk_size, i+1))
            sys.stdout.flush()
        sys.stdout.write('\n')

if __name__ == '__main__':
    main()
