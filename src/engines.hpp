#pragma once

void engine_select(int sfd, int backlog);
void engine_poll(int sfd, int backlog);
void engine_epoll(int sfd, int backlog);
