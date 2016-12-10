#pragma once

void engine_naive(int sfd, int backlog);
void engine_fork(int sfd, int backlog);
void engine_thread(int sfd, int backlog);
void engine_pool(int sfd, int backlog);
void engine_select(int sfd, int backlog);
void engine_poll(int sfd, int backlog);
void engine_epoll(int sfd, int backlog);
