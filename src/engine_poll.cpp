#include <cstring>
#include <cstdlib>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <poll.h>
#include <sys/socket.h>
#include <unistd.h>
#include "http.hpp"
#include "engines.hpp"
#include "network.hpp"

void engine_poll(int sfd, int backlog) {
    make_socket_non_blocking(sfd);
    HTTPRequest *reqs = new HTTPRequest[backlog];
    struct pollfd *pollfds = static_cast<struct pollfd *>(std::calloc(sizeof(struct pollfd), backlog));
    pollfds[0].fd = sfd;
    pollfds[0].events = POLLIN;
    int nfds = 1;
    for (;;) {
        if (poll(pollfds, nfds, -1) < 0) {
            perror("poll");
            abort();
        }
        for (int i = 0; i < nfds; ++i) {
            struct pollfd *pfd = &pollfds[i];
            if (pfd->revents == 0)
                continue;
            if (!(pfd->revents & POLLIN))
            {
                // socket exception
                fprintf(stderr, "exception fd = %d, POLLERR=%d, POLLHUP=%d, POLLNVAL=%d\n", pfd->fd, pfd->revents & POLLERR, pfd->revents & POLLHUP, pfd->revents & POLLNVAL);
                close_request(&reqs[pfd->fd]);
                memcpy(&pollfds[i--], &pollfds[--nfds], sizeof(struct pollfd));
                continue;
            }
            if (pfd->fd == sfd) {
                // new connection
                for (;;) {
                    int infd = accept_connection(sfd);
                    if (infd < 0) break;
                    pollfds[nfds].fd = infd;
                    pollfds[nfds].events = POLLIN;
                    ++nfds;
                    HTTPRequest *r = &reqs[infd];
                    r->clear();
                    r->fd_socket = infd;
                }
            } else {
                // new data
                HTTPRequest *r = &reqs[pfd->fd];
                DoRequestResult res = do_request(r);
                switch (res) {
                    case DO_REQUEST_AGAIN:
                        break;
                    case DO_REQUEST_CLOSE:
                        close_request(r);
                        memcpy(&pollfds[i--], &pollfds[--nfds], sizeof(struct pollfd));
                        break;
                    default:
                        abort();
                }
            }
        }
    }
}
