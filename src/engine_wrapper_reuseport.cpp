#include <cstdio>
#include <cstdlib>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>
#include "engines.hpp"
#include "network.hpp"

void engine_wrapper_reuseport(EngineFunction engine, int port, int backlog, int num_worker) {
    for (int i = 0; i < num_worker; ++i) {
        pid_t pid = fork();
        if (pid < 0) {
            perror("fork");
            abort();
        } else if (pid == 0) {
            // child
            int sfd = create_and_bind(port, true);
            if (listen(sfd, backlog) < 0) {
                perror("listen");
                abort();
            }
            engine(sfd, backlog, 1);
            return;
        }
    }
    for (int i = 0; i < num_worker; ++i)
        wait(NULL);
}
