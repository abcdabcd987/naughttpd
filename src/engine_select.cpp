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

void engine_select(int sfd, int backlog, int _) {
    make_socket_non_blocking(sfd);
    fprintf(stderr, "FD_SETSIZE = %d\n", FD_SETSIZE);
    HTTPRequest *reqs = new HTTPRequest[backlog];
    fd_set rfds, wfds, efds;
    fd_set readfds, writefds, exceptfds;
    int maxsock = sfd;
    FD_ZERO(&rfds);
    FD_ZERO(&wfds);
    FD_ZERO(&efds);
    FD_SET(sfd, &rfds);
    FD_SET(sfd, &efds);
    for (;;) {
        memcpy(&readfds, &rfds, sizeof(fd_set));
        memcpy(&writefds, &wfds, sizeof(fd_set));
        memcpy(&exceptfds, &efds, sizeof(fd_set));
        if (select(maxsock+1, &readfds, &writefds, &exceptfds, NULL) < 0) {
            perror("select");
            abort();
        }
        for (int i = 0; i <= maxsock; ++i) {
            if (FD_ISSET(i, &exceptfds)) {
                // socket exception
                HTTPRequest *r = &reqs[i];
                FD_CLR(i, &rfds);
                FD_CLR(i, &wfds);
                FD_CLR(i, &efds);
                close_request(r);
                continue;
            }
            if (!FD_ISSET(i, &readfds) && !FD_ISSET(i, &writefds))
                continue;
            if (i == sfd) {
                // new connection
                for (;;) {
                    int infd = accept_connection(sfd);
                    if (infd < 0) break;
                    FD_SET(infd, &rfds);
                    FD_SET(infd, &efds);
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
                    case DO_REQUEST_READ_AGAIN:
                        FD_SET(i, &rfds);
                        FD_CLR(i, &wfds);
                        break;
                    case DO_REQUEST_WRITE_AGAIN:
                        FD_CLR(i, &rfds);
                        FD_SET(i, &wfds);
                        break;
                    case DO_REQUEST_CLOSE:
                        FD_CLR(i, &rfds);
                        FD_CLR(i, &wfds);
                        FD_CLR(i, &efds);
                        close_request(r);
                        break;
                    default:
                        abort();
                }
            }
        }
    }
}