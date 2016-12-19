#include <cstring>
#include <cstdlib>
#include <errno.h>
#include <netdb.h>
#include <sys/socket.h>
#include <unistd.h>
#include "http.hpp"
#include "engines.hpp"
#include "network.hpp"

void engine_fork(int sfd, int backlog, int num_worker) {
    for (;;) {
        int infd = accept(sfd, NULL, NULL);
        if (infd < 0) {
            perror("accept");
            abort();
        }
        pid_t pid = fork();
        if (pid < 0) {
            perror("fork");
            abort();
        } else if (pid == 0) {
            // child
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
        } else {
            // parent
            close(infd);
        }
    }
}
