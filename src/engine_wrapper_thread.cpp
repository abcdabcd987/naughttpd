#include <cstdio>
#include <cstdlib>
#include <errno.h>
#include <pthread.h>
#include "engines.hpp"

struct _worker_arg_t {
    EngineFunction engine;
    int sfd;
    int backlog;
    int num_worker;
};

static void* worker(void *args) {
    _worker_arg_t *p = static_cast<_worker_arg_t*>(args);
    p->engine(p->sfd, p->backlog, 1);
}

void engine_wrapper_thread(EngineFunction engine, int sfd, int backlog, int num_worker) {
    _worker_arg_t args = { engine, sfd, backlog, num_worker };
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
