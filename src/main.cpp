#include "http.hpp"
#include "network.hpp"
#include <signal.h>

const int MAXEVENT = 1024;
struct epoll_event events[MAXEVENT];

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "usage: %s port\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // set up
    signal(SIGPIPE, SIG_IGN);
    int sfd = create_and_bind(argv[1]);
    make_socket_non_blocking(sfd);
    if (listen(sfd, SOMAXCONN) < 0) {
        perror("listen");
        abort();
    }
    int efd = epoll_create1(0);
    if (efd < 0) {
        perror("epoll_create1");
        abort();
    }

    // use HTTPRequest simply to store sfd and efd
    struct epoll_event event;
    event.data.ptr = static_cast<void*>(new HTTPRequest(sfd, efd));
    event.events = EPOLLIN | EPOLLET;
    if (epoll_ctl(efd, EPOLL_CTL_ADD, sfd, &event) < 0) {
        perror("epoll_ctl");
        abort();
    }

    // the event loop
    for (;;) {
        int n = epoll_wait(efd, events, MAXEVENTS, -1);
        for (int i = 0; i < n; ++i) {
            HTTPRequest *r = static_cast<HTTPRequest*>(events[i].data.ptr);

            if ((events[i].events & EPOLLERR) ||
                (events[i].events & EPOLLHUP) ||
                (!(events[i].events & EPOLLIN)))
            {
                // An error has occured on this fd, or the socket is not
                // ready for reading (why were we notified then?)
                fprintf(stderr, "epoll error\n");
                close_request(r);
                continue;
            }

            if (sfd == r->fd_socket) {
                // We have a notification on the listening socket, which
                // means one or more incoming connections.
                for (;;) {
                    struct sockaddr in_addr;
                    socklen_t in_len = sizeof in_addr;
                    char hbuf[NI_MAXHOST], sbuf[NI_MAXSERV];
                    int infd = accept(sfd, &in_addr, &in_len);
                    if (infd < 0) {
                        if (errno == EAGAIN || errno == EWOULDBLOCK) {
                            break; //We have processed all incoming connections.
                        } else {
                            perror("accept");
                            abort();
                        }
                    }
                    fprintf(stderr, "accept  fd=%d\n", infd);
                    make_socket_non_blocking(infd);
                    event.data.ptr = static_cast<void*>(new HTTPRequest(infd, efd));
                    event.events = EPOLLIN | EPOLLET | EPOLLONESHOT;
                    if (epoll_ctl(efd, EPOLL_CTL_ADD, infd, &event) < 0) {
                        perror("epoll_ctl");
                        abort();
                    }
                }
            } else {
                // We have data on the fd waiting to be read. Read and
                // display it. We must read whatever data is available
                // completely, as we are running in edge-triggered mode
                // and won't get a notification again for the same
                // data.
                do_request(r);
            }
        }
    }
}
