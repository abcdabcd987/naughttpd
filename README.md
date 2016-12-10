# naughttpd
A naught http server using reactor pattern.

## `wrk` examples

```
wrk -t1 -c1000 -d10s http://localhost:8080/README.md
wrk -t1 -c1000 -d10s --header "Connection: Close" http://localhost:8080/README.md
```
