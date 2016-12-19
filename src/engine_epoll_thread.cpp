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

struct _worker_arg_t {
    int sfd;
    int backlog;
    int num_worker;
};

static void* worker(void *args) {
    _worker_arg_t *p = static_cast<_worker_arg_t*>(args);
    engine_epoll(p->sfd, p->backlog, p->num_worker);
}

void engine_epoll_thread(int sfd, int backlog, int num_worker) {
    _worker_arg_t args = { sfd, backlog, num_worker };
    pthread_t threads[num_worker];
    for (int i = 0; i < num_worker; ++i)
        if (pthread_create(&threads[i], NULL, worker, static_cast<void*>(&args)) < 0) {
            perror("pthread_create");
            abort();
        }
    for (int i = 0; i < num_worker; ++i)
        if (pthread_join(threads[i], NULL) < 0) {
            perror("pthread_join");
            abort();
        }
}
