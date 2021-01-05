#include "header.h"

struct info_t {
public:
    uint64_t val;

public:
    info_t()
        : val(0)
    {
    }

public:
    void lock()
    {
    }

    void unlock()
    {
    }
};

int g_num_thread = 8;

uint64_t g_count = 100000000UL;

int g_core[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };

static void run_thread(int thread_id, info_t* info)
{
    cpu_set_t _mask;
    CPU_ZERO(&_mask);
    CPU_SET(g_core[thread_id], &_mask);
    if (pthread_setaffinity_np(pthread_self(), sizeof(_mask), &_mask) < 0) {
        printf("threadpool, set thread affinity failed.\n");
    }

    int _cnt = g_count;
    Timer _timer;

    _timer.Start();
    for (uint64_t i = 0; i < _cnt; i++) {
        info->lock();
        info->val += 10;
        info->unlock();
    }
    _timer.Stop();
    printf("[thread%02d][time:%lluns/%.2fus/%.2fsec]\n", _timer.Get(), 1.0 * _timer.Get() / 1000, 1.0 * _timer.Get() / (1000000000));
}

int main(int argc, char** argv)
{
    Timer _timer;
    info_t _info;
    std::thread _threads[16];

    _timer.Start();
    for (int i = 0; i < g_num_thread; i++) {
        _threads[i] = std::thread(run_thread, i, &_info);
    }
    for (int i = 0; i < g_num_thread; i++) {
        _threads[i].join();
    }
    _timer.Stop();
    printf("[Finished][val:%llu][time:%lluns/%.2fus/%.2fsec]\n", _info.val, _timer.Get(), 1.0 * _timer.Get() / 1000, 1.0 * _timer.Get() / (1000000000));
    return 0;
}