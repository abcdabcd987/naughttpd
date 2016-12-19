#pragma once

void engine_naive(int sfd, int backlog, int num_worker);
void engine_fork(int sfd, int backlog, int num_worker);
void engine_thread(int sfd, int backlog, int num_worker);
void engine_pool(int sfd, int backlog, int num_worker);
void engine_select(int sfd, int backlog, int num_worker);
void engine_poll(int sfd, int backlog, int num_worker);
void engine_epoll(int sfd, int backlog, int num_worker);
void engine_epoll_thread(int sfd, int backlog, int num_worker);
void engine_epoll_reuseport(int port, int backlog, int num_worker);
