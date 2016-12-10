#pragma once
#include <queue>
#include <vector>
#include <utility>

class ThreadPool {
    size_t num_worker;
    pthread_mutex_t lock;
    pthread_cond_t notify;
    std::vector<pthread_t> threads;
    std::queue<std::pair<void (*)(void*), void*>> queue;
    friend void *thread_pool_work(void *pool);

public:
    ThreadPool(size_t num_worker_);
    void add(void (*func)(void *), void *arg);
};
