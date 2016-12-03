#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/epoll.h>

#define MAXEVENTS 64

int create_and_bind(char *port);
void make_socket_non_blocking(int sfd);
ssize_t rio_writen(int fd, const void *usrbuf, size_t n);
