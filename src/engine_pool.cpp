#include <cstring>
#include <cstdlib>
#include <errno.h>
#include <netdb.h>
#include <pthread.h>
#include <sys/socket.h>
#include <unistd.h>
#include "http.hpp"
#include "engines.hpp"
#include "network.hpp"
#include "threadpool.hpp"

static void process(void *arg) {
    int infd = reinterpret_cast<intptr_t>(arg);
    HTTPRequest r;
    for (;;) {
        r.clear();
        r.fd_socket = infd;
        DoRequestResult res = do_request(&r);
        if (res == DO_REQUEST_CLOSE) {
            close_request(&r);
            break;
        }
    }
}

void engine_pool(int sfd, int backlog, int num_worker) {
    fprintf(stderr, "num_worker = %d\n", num_worker);
    ThreadPool pool(static_cast<size_t>(num_worker));
    for (;;) {
        int infd = accept(sfd, NULL, NULL);
        if (infd < 0) {
            perror("accept");
            abort();
        }
        pool.add(process, reinterpret_cast<void*>(infd));
    }
}
