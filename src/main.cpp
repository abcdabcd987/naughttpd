#include <cstring>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include "http.hpp"
#include "engines.hpp"
#include "network.hpp"

typedef void (*EngineFunction)(int, int, int);
struct EngineTuple {
    const char *name;
    EngineFunction func;
    bool multiworker;
    bool listen;
};
const EngineTuple engines[] = {
    { "naive"          , engine_naive          , false, true  },
    { "fork"           , engine_fork           , false, true  },
    { "thread"         , engine_thread         , false, true  },
    { "pool"           , engine_pool           , true , true  },
    { "select"         , engine_select         , false, true  },
    { "poll"           , engine_poll           , false, true  },
    { "epoll"          , engine_epoll          , false, true  },
    { "epoll_thread"   , engine_epoll_thread   , true , true  },
    { "epoll_reuseport", engine_epoll_reuseport, true , false },
};

void help(int argc, char *argv[]) {
    fprintf(stderr, "usage: %s port engine num_worker backlog tcp_cork\n", argv[0]);
    fprintf(stderr, "e.g. %s 8080 epoll_thread 4 511 on\n", argv[0]);
    fprintf(stderr, "available engines:\n");
    for (size_t i = 0; i < sizeof(engines) / sizeof(EngineTuple); ++i)
        fprintf(stderr, "       %s\n", engines[i].name);
    exit(EXIT_FAILURE);
}

extern bool enable_tcp_cork;

int main(int argc, char *argv[]) {
    if (argc != 6)
        help(argc, argv);
    const EngineTuple *e = NULL;
    for (size_t i = 0; i < sizeof(engines) / sizeof(EngineTuple); ++i)
        if (strcmp(engines[i].name, argv[2]) == 0)
            e = &engines[i];
    int num_worker = std::atoi(argv[3]);
    enable_tcp_cork = strcmp(argv[5], "on") == 0;
    if (!e || (num_worker != 1 && !e->multiworker))
        help(argc, argv);

    // listen
    int backlog = std::atoi(argv[4]);
    fprintf(stderr, "listen backlog = %d\n", backlog);
    fprintf(stderr, "tcp_cork %s\n", enable_tcp_cork ? "on" : "off");
    signal(SIGPIPE, SIG_IGN);
    int sfd = -1;
    if (e->listen) {
        sfd = create_and_bind(argv[1], false);
        if (listen(sfd, backlog) < 0) {
            perror("listen");
            abort();
        }
    } else {
        sfd = std::atoi(argv[1]);
    }

    // run
    e->func(sfd, backlog, num_worker);
}
