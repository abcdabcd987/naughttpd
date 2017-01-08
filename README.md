# naughttpd
A naive static http server that solves C10K problem.

## More detailed report?

See `doc/report.pdf` (in Chinese)

## How to compile?

```bash
make
```

## Usage?

```
usage: naughttpd --port <int>
                 --engine <naive/fork/thread/pool/select/poll/epoll>
                 --backlog <int>
                 --num_workers <int>
                 --reuseport <on/off>
                 --tcp_cork <on/off>
                 --tcp_nodelay <on/off>
                 --sendfile <naive/mmap/sendfile>
```

