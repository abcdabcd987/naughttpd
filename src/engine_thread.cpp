#include <cstring>
#include <cstdlib>
#include <errno.h>
#include <netdb.h>
#include <sys/socket.h>
#include <unistd.h>
#include "http.hpp"
#include "engines.hpp"
#include "network.hpp"

void *process(void *arg) {
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
    return NULL;
}

void engine_thread(int sfd, int backlog, int _) {
    for (;;) {
        int infd = accept(sfd, NULL, NULL);
        if (infd < 0) {
            perror("accept");
            abort();
        }
        pthread_t thread;
        pthread_attr_t attr;
        if (pthread_attr_init(&attr) < 0) {
            perror("pthread_attr_init");
            abort();
        }
        if (pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED) < 0) {
            perror("pthread_attr_setdetachstate");
            abort();
        }
        if (pthread_create(&thread, &attr, process, reinterpret_cast<void*>(infd)) < 0) {
            perror("pthread_create");
            abort();
        }
        if (pthread_attr_destroy(&attr) < 0) {
            perror("pthread_attr_destroy");
            abort();
        }
    }
}
