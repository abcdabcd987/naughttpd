#include <cstring>
#include <cstdlib>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <unistd.h>
#include "http.hpp"
#include "engines.hpp"
#include "network.hpp"

void engine_epoll(int sfd, int backlog, int num_worker) {
    make_socket_non_blocking(sfd);
    struct epoll_event *events = static_cast<struct epoll_event*>(std::calloc(sizeof(struct epoll_event), backlog));
    int efd = epoll_create1(0);
    if (efd < 0) {
        perror("epoll_create1");
        abort();
    }

    // use HTTPRequest simply to store sfd and efd
    struct epoll_event event;
    HTTPRequest *fake_req = new HTTPRequest;
    fake_req->fd_socket = sfd;
    fake_req->ngdata.fd_epoll = efd;
    event.data.ptr = static_cast<void*>(fake_req);
    event.events = EPOLLIN | EPOLLET;
    if (epoll_ctl(efd, EPOLL_CTL_ADD, sfd, &event) < 0) {
        perror("epoll_ctl");
        abort();
    }

    // the event loop
    for (;;) {
        int n = epoll_wait(efd, events, backlog, -1);
        for (int i = 0; i < n; ++i) {
            HTTPRequest *r = static_cast<HTTPRequest*>(events[i].data.ptr);

            if ((events[i].events & EPOLLERR) ||
                (events[i].events & EPOLLHUP) ||
                (!(events[i].events & EPOLLIN)))
            {
                // An error has occured on this fd, or the socket is not
                // ready for reading (why were we notified then?)
                // fprintf(stderr, "epoll error\n");
                close_request(r);
                delete r;
                continue;
            }

            if (sfd == r->fd_socket) {
                // We have a notification on the listening socket, which
                // means one or more incoming connections.
                for (;;) {
                    int infd = accept_connection(sfd);
                    if (infd < 0) break;
                    HTTPRequest *req = new HTTPRequest;
                    req->fd_socket = infd;
                    req->ngdata.fd_epoll = efd;
                    event.data.ptr = static_cast<void*>(req);
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
                DoRequestResult res = do_request(r);
                switch (res) {
                    case DO_REQUEST_AGAIN:
                        event.data.ptr = static_cast<void*>(r);
                        event.events = EPOLLIN | EPOLLET | EPOLLONESHOT;
                        if (epoll_ctl(r->ngdata.fd_epoll, EPOLL_CTL_MOD, r->fd_socket, &event) < 0) {
                            perror("epoll_ctl");
                            abort();
                        }
                        break;
                    case DO_REQUEST_CLOSE:
                        // closing a file descriptor will cause it to be removed from all epoll sets automatically
                        // see: http://stackoverflow.com/questions/8707601/is-it-necessary-to-deregister-a-socket-from-epoll-before-closing-it
                        close_request(r);
                        delete r;
                        break;
                    default:
                        abort();
                }
            }
        }
    }
}
