#include <cstring>
#include <cstdlib>
#include <errno.h>
#include <netdb.h>
#include <sys/socket.h>
#include <unistd.h>
#include "http.hpp"
#include "engines.hpp"
#include "network.hpp"

void engine_naive(int sfd, int backlog, int num_worker) {
    HTTPRequest r;
    for (;;) {
        int infd = accept(sfd, NULL, NULL);
        if (infd < 0) {
            perror("accept");
            abort();
        }
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
}
