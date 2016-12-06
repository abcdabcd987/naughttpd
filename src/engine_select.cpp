#include <cstring>
#include <cstdlib>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>
#include "http.hpp"
#include "engines.hpp"
#include "network.hpp"

void engine_select(int sfd, int backlog) {
    fprintf(stderr, "FD_SETSIZE = %d\n", FD_SETSIZE);
    HTTPRequest *reqs = new HTTPRequest[backlog];
    fd_set socks, readfs, exceptfds;
    int maxsock = sfd;
    FD_ZERO(&socks);
    FD_SET(sfd, &socks);
    for (;;) {
        memcpy(&readfs, &socks, sizeof(socks));
        memcpy(&exceptfds, &socks, sizeof(socks));
        if (select(maxsock+1, &readfs, NULL, &exceptfds, NULL) < 0) {
            perror("select");
            abort();
        }
        for (int i = 0; i <= maxsock; ++i) {
            if (FD_ISSET(i, &exceptfds)) {
                // socket exception
                fprintf(stderr, "exception fd = %d\n", i);
                HTTPRequest *r = &reqs[i];
                FD_CLR(i, &socks);
                close_request(r);
                continue;
            }
            if (!FD_ISSET(i, &readfs))
                continue;
            if (i == sfd) {
                // new connection
                for (;;) {
                    int infd = accept_connection(sfd);
                    if (infd < 0) break;
                    FD_SET(infd, &socks);
                    if (infd > maxsock) maxsock = infd;
                    HTTPRequest *r = &reqs[infd];
                    r->clear();
                    r->fd_socket = infd;
                }
            } else {
                // new data
                HTTPRequest *r = &reqs[i];
                DoRequestResult res = do_request(r);
                switch (res) {
                    case DO_REQUEST_AGAIN:
                        break;
                    case DO_REQUEST_CLOSE:
                        FD_CLR(i, &socks);
                        close_request(r);
                        break;
                    default:
                        abort();
                }
            }
        }
    }
}