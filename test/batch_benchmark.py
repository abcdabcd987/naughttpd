from collections import namedtuple
import os
import sys
import time
import subprocess

Setting = namedtuple('Setting', ['engine', 'backlog', 'num_workers', 'reuseport', 'tcp_nodelay', 'tcp_cork', 'sendfile', 'concurrency', 'filename'])

def main():
    if len(sys.argv) != 4:
        sys.stderr.write('usage: python test/batch_benchmark.py <local ip> <wrk host ip> <datapoints.csv>\n')
        sys.exit(1)
    localip = sys.argv[1]
    wrkip = sys.argv[2]
    port = 8080
    wrk_thread = 16
    wrk_duration = 30

    settings = []
    with open(sys.argv[3]) as f:
        f.next()
        for line in f:
            settings.append(Setting._make(line.strip().split(',')))

    devnull = open(os.devnull, 'w')
    sys.stdout.write('engine,backlog,num_workers,reuseport,tcp_nodelay,tcp_cork,sendfile,concurrency,filename,latency(ms),requests/sec\n')
    for setting in settings:
        cmd_httpd = ['bin/naughttpd',
            '--port', str(port),
            '--engine', setting.engine,
            '--backlog', setting.backlog,
            '--num_workers', setting.num_workers,
            '--reuseport', setting.reuseport,
            '--tcp_cork', setting.tcp_cork,
            '--tcp_nodelay', setting.tcp_nodelay,
            '--sendfile', setting.sendfile
        ]
        cmd_wrk = ['ssh', wrkip, 'wrk',
            '-t', str(wrk_thread),
            '-d', '%ss' % wrk_duration,
            '-c', setting.concurrency,
            'http://%s:%d/%s' % (localip, port, setting.filename)
        ]

        httpd = subprocess.Popen(cmd_httpd, stdin=devnull, stdout=devnull, stderr=devnull)
        time.sleep(2)
        wrk = subprocess.Popen(cmd_wrk, stdin=devnull, stdout=subprocess.PIPE, stderr=devnull)
        text, _ = wrk.communicate()
        httpd.kill()
        wrk.wait()
        httpd.wait()

        ts = text.split()
        latency = ts[ts.index('Latency') + 1]
        if latency.endswith('us'):
            latency = str(float(latency[:-2]) / 1000.)
        else:
            latency = latency[:-2]
        requests = ts[ts.index('Requests/sec:') + 1]

        sys.stdout.write(','.join(list(setting) + [latency, requests]) + '\n')
        sys.stdout.flush()

if __name__ == '__main__':
    main()
