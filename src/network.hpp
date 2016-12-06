#pragma once
#include <sys/types.h>

int create_and_bind(char *port);
void make_socket_non_blocking(int sfd);
ssize_t rio_writen(int fd, const void *usrbuf, size_t n);
int accept_connection(int sfd);
