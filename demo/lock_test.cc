#include "header.h"
#include <mutex>
#include <pthread.h>

#define NO_LOCK

// #define USE_MUTEXT_LOCK

// #define USE_ATOMIC_LOCK

// #define USE_PTHREAD_SPINTLOCK

#define USE_PTHREAD_RWLOCK

struct info_t {
public:
    uint64_t val;
#ifdef USE_MUTEXT_LOCK
    std::mutex _mutex;
#elif defined(USE_ATOMIC_LOCK)
    std::atomic<int> _atomic_flag;
#elif defined(USE_PTHREAD_SPINTLOCK)
    pthread_spinlock_t _spinlock;
#elif defined(USE_PTHREAD_RWLOCK)
    pthread_rwlock_t _rwlock;
#endif

public:
    info_t()
        : val(0)
    {
#ifdef USE_MUTEXT_LOCK
#elif defined(USE_ATOMIC_LOCK)
        _atomic_flag = 0;
#elif defined(USE_PTHREAD_SPINTLOCK)
#elif defined(USE_PTHREAD_RWLOCK)
        pthread_rwlock_init(&_rwlock, 0);
#endif
    }

public:
    void lock()
    {
#ifdef USE_MUTEXT_LOCK
        _mutex.lock();
#elif defined(USE_ATOMIC_LOCK)
        int _status = 0;
        while (!_atomic_flag.compare_exchange_strong(_status, 1, std::memory_order_acquire)) {
        }
#elif defined(USE_PTHREAD_SPINTLOCK)
        pthread_spin_lock(&_spinlock);
#elif defined(USE_PTHREAD_RWLOCK)
        pthread_rwlock_wrlock(&_rwlock);
#endif
    }

    void unlock()
    {
#ifdef USE_MUTEXT_LOCK
        _mutex.unlock();
#elif defined(USE_ATOMIC_LOCK)
        _atomic_flag = 0;
#elif defined(USE_PTHREAD_SPINTLOCK)
        pthread_spin_unlock(&_spinlock);
#elif defined(USE_PTHREAD_RWLOCK)
        pthread_rwlock_unlock(&_rwlock);
#endif
    }
};

int g_num_thread = 8;

uint64_t g_count = 10000000UL;

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
    printf("[thread%02d][time:%lluns/%.2fus/%.2fsec]\n", thread_id, _timer.Get(), 1.0 * _timer.Get() / 1000, 1.0 * _timer.Get() / (1000000000));
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