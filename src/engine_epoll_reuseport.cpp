#include <cstring>
#include <cstdlib>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include "http.hpp"
#include "engines.hpp"
#include "network.hpp"

void engine_epoll_reuseport(int port, int backlog, int num_worker) {
    char str_port[10];
    snprintf(str_port, 10, "%d", port);
    for (int i = 0; i < num_worker; ++i) {
        pid_t pid = fork();
        if (pid < 0) {
            perror("fork");
            abort();
        } else if (pid == 0) {
            // child
            int sfd = create_and_bind(str_port, true);
            if (listen(sfd, backlog) < 0) {
                perror("listen");
                abort();
            }
            engine_epoll(sfd, backlog, num_worker);
            return;
        }
    }
    for (int i = 0; i < num_worker; ++i)
        wait(NULL);
}
