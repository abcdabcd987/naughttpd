#pragma once

typedef void (*EngineFunction)(int, int, int);
void engine_naive(int sfd, int backlog, int _);
void engine_fork(int sfd, int backlog, int _);
void engine_thread(int sfd, int backlog, int _);
void engine_pool(int sfd, int backlog, int num_worker);
void engine_select(int sfd, int backlog, int _);
void engine_poll(int sfd, int backlog, int _);
void engine_epoll(int sfd, int backlog, int _);

void engine_wrapper_thread(EngineFunction engine, int sfd, int backlog, int num_worker);
void engine_wrapper_reuseport(EngineFunction engine, int port, int backlog, int num_worker);
