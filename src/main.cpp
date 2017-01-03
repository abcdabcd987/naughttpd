#include <cstring>
#include <getopt.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include "http.hpp"
#include "engines.hpp"
#include "network.hpp"
#include "cmdopts.hpp"

struct EngineTuple {
    const char *name;
    EngineFunction func;
    bool multiworker;
    bool threaded;
    bool reuseport;
};
const EngineTuple engines[] = {
    //  name  ,  func        , multi,thread, reuse
    { "naive" , engine_naive , false, false, false  },
    { "fork"  , engine_fork  , false, false, false  },
    { "thread", engine_thread, false, false, false  },
    { "pool"  , engine_pool  , true , false, false  },
    { "select", engine_select, false, true , true   },
    { "poll"  , engine_poll  , false, true , true   },
    { "epoll" , engine_epoll , false, true , true   },
};

void help(int argc, char *argv[]) {
    fprintf(stderr, "usage: naughttpd --port <int>\n"
                    "                 --engine <string>\n"
                    "                 --backlog <int>\n"
                    "                 --num_workers <int>\n"
                    "                 --reuseport <on/off>\n"
                    "                 --tcp_cork <on/off>\n"
                    "                 --tcp_nodelay <on/off>\n"
                    "                 --sendfile <naive/mmap/sendfile>\n");
    fprintf(stderr, "available engines:\n");
    for (size_t i = 0; i < sizeof(engines) / sizeof(EngineTuple); ++i) {
        const EngineTuple *e = &engines[i];
        fprintf(stderr, "       %-10s%30s%30s\n",
            e->name,
            e->multiworker || e->threaded ? "support multiple workers" : "",
            e->reuseport ? "support reuseport" : "");
    }
    exit(EXIT_FAILURE);
}

int main(int argc, char *argv[]) {
    CommandLineOptions args(argc, argv);
    int port = std::stoi(args.get("port", "8080"));
    int num_workers = std::stoi(args.get("num_workers", "1"));
    enable_tcp_cork = args.get("tcp_cork", "off") == "on";
    enable_tcp_nodelay = args.get("tcp_nodelay", "off") == "on";
    std::string engine = args.get("engine");
    int backlog = std::stoi(args.get("backlog", "511"));
    bool reuseport = args.get("reuseport", "off") == "on";
    if (args.get("sendfile") == "sendfile")
        sendfile_method = SENDFILE_SENDFILE;
    else if (args.get("sendfile") == "mmap")
        sendfile_method = SENDFILE_MMAP;
    else
        sendfile_method = SENDFILE_NAIVE;

    const EngineTuple *e = NULL;
    for (size_t i = 0; i < sizeof(engines) / sizeof(EngineTuple); ++i)
        if (strcmp(engines[i].name, engine.c_str()) == 0)
            e = &engines[i];
    if (!e ||
        (num_workers != 1 && !e->multiworker && !e->threaded) ||
        (reuseport && !e->reuseport))
        help(argc, argv);
    fprintf(stderr, "%d(%d) %s*%d %s%s%s%s\n",
        port, backlog,
        e->name, num_workers,
        reuseport ? "reuseport " : "",
        enable_tcp_cork ? "tcp_cork " : "",
        enable_tcp_nodelay ? "tcp_nodelay " : "",
        sendfile_method == SENDFILE_SENDFILE ? "sendfile " :
            (sendfile_method == SENDFILE_MMAP ? "mmap " : ""));

    // listen
    signal(SIGPIPE, SIG_IGN);
    int sfd = -1;
    if (!reuseport) {
        sfd = create_and_bind(port, false);
        if (listen(sfd, backlog) < 0) {
            perror("listen");
            abort();
        }
    }

    // run
    if (reuseport)
        engine_wrapper_reuseport(e->func, port, backlog, num_workers);
    else if (num_workers != 1)
        engine_wrapper_thread(e->func, sfd, backlog, num_workers);
    else
        e->func(sfd, backlog, num_workers);
}
