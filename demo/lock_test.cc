#include "header.h"
#include "tbb/tbb/spin_mutex.h"
#include "tbb/tbb/spin_rw_mutex.h"
#include <mutex>
#include <pthread.h>

#define NUM_THREADS (32)

// #define ATOMIC_ADD

#define USE_MUTEXT_LOCK

// #define USE_ATOMIC_LOCK

// #define USE_PTHREAD_SPINTLOCK

// #define USE_PTHREAD_RWLOCK

// #define USE_TBB_MUTEXT_LOCK

// #define USE_TBB_RWLOCK

/*
typedef enum memory_order {
    memory_order_relaxed,   // 不对执行顺序做保证
    memory_order_acquire,   // 本线程中,所有后续的读操作必须在本条原子操作完成后执行
    memory_order_release,   // 本线程中,所有之前的写操作完成后才能执行本条原子操作
    memory_order_acq_rel,   // 同时包含 memory_order_acquire 和 memory_order_release
    memory_order_consume,   // 本线程中,所有后续的有关本原子类型的操作,必须在本条原子操作完成之后执行
    memory_order_seq_cst    // 全部存取都按顺序执行
    } memory_order;
*/

struct info_t {
public:
#ifdef ATOMIC_ADD
    std::atomic<uint64_t> val;
#else
    uint64_t val;
#endif

#ifdef USE_MUTEXT_LOCK
    std::mutex _mutex;
#elif defined(USE_ATOMIC_LOCK)
    std::atomic<int> _atomic_flag;
#elif defined(USE_PTHREAD_SPINTLOCK)
    pthread_spinlock_t _spinlock;
#elif defined(USE_PTHREAD_RWLOCK)
    pthread_rwlock_t _rwlock;
#elif defined(USE_TBB_MUTEXT_LOCK)
    tbb::spin_mutex _tbb_mutex;
#elif defined(USE_TBB_RWLOCK)
    tbb::spin_rw_mutex _tbb_rwlock;
#endif
    Timer _timer[NUM_THREADS];

public:
    info_t()
        : val(0)
    {
#ifdef USE_MUTEXT_LOCK
        printf("std_mutext test\n");
#elif defined(USE_ATOMIC_LOCK)
        _atomic_flag.store(0);
        printf("atomic lock test\n");
#elif defined(USE_PTHREAD_SPINTLOCK)
        pthread_spin_init(&_spinlock, PTHREAD_PROCESS_PRIVATE);
        printf("pthread_spinlock test\n");
#elif defined(USE_PTHREAD_RWLOCK)
        pthread_rwlock_init(&_rwlock, 0);
        printf("pthread_rwlock test\n");
#elif defined(USE_TBB_MUTEXT_LOCK)
        printf("tbb_mutext test\n");
#elif defined(USE_TBB_RWLOCK)
        printf("tbb_rwlock test\n");
#elif defined(ATOMIC_ADD)
        printf("atomic add\n");
#endif
    }

public:
    void lock()
    {
#ifdef USE_MUTEXT_LOCK
        _mutex.lock();
#elif defined(USE_ATOMIC_LOCK)
        int _status = 0;
        while (!_atomic_flag.compare_exchange_strong(_status, 1)) {
            _status = 0;
        }
#elif defined(USE_PTHREAD_SPINTLOCK)
        pthread_spin_lock(&_spinlock);
#elif defined(USE_PTHREAD_RWLOCK)
        pthread_rwlock_wrlock(&_rwlock);
#elif defined(USE_TBB_MUTEXT_LOCK)
        _tbb_mutex.lock();
#elif defined(USE_TBB_RWLOCK)
        _tbb_rwlock.lock();
#elif defined(ATOMIC_ADD)
#endif
    }

    void unlock()
    {
#ifdef USE_MUTEXT_LOCK
        _mutex.unlock();
#elif defined(USE_ATOMIC_LOCK)
        _atomic_flag.store(0);
#elif defined(USE_PTHREAD_SPINTLOCK)
        pthread_spin_unlock(&_spinlock);
#elif defined(USE_PTHREAD_RWLOCK)
        pthread_rwlock_unlock(&_rwlock);
#elif defined(USE_TBB_MUTEXT_LOCK)
        _tbb_mutex.unlock();
#elif defined(USE_TBB_RWLOCK)
        _tbb_rwlock.unlock();
#elif defined(ATOMIC_ADD)
#endif
    }

    void add10()
    {
        val += 10;
    }
};

int g_num_thread = 2;

uint64_t g_count = 10000000UL;

int g_core[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29 };

#define NOP10() asm("nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;")

static void run_thread(int thread_id, info_t* info)
{
    cpu_set_t _mask;
    CPU_ZERO(&_mask);
    CPU_SET(g_core[thread_id], &_mask);
    if (pthread_setaffinity_np(pthread_self(), sizeof(_mask), &_mask) < 0) {
        printf("threadpool, set thread affinity failed.\n");
    }

    int _cnt = g_count;
    Timer* _timer = &info->_timer[thread_id];
    _timer->Start();
    for (uint64_t i = 0; i < _cnt; i++) {
        info->lock();
        info->add10();
        info->unlock();
        // NOP10();
    }
    _timer->Stop();
    printf("[thread%02d][time:%lluns/%.2fus/%.2fsec]\n", thread_id, _timer->Get(), 1.0 * _timer->Get() / 1000, 1.0 * _timer->Get() / (1000000000));
}

int main(int argc, char** argv)
{
    Timer _timer;
    info_t _info;
    std::thread _threads[16];

    g_num_thread = atol(argv[1]);

    _timer.Start();
    for (int i = 0; i < g_num_thread; i++) {
        _threads[i] = std::thread(run_thread, i, &_info);
    }
    for (int i = 0; i < g_num_thread; i++) {
        _threads[i].join();
    }
    _timer.Stop();

    uint64_t _total_time = 0;
    double _avg_time;
    for (int i = 0; i < g_num_thread; i++) {
        _total_time += _info._timer[i].Get();
    }
    _avg_time = 1.0 * _total_time / g_num_thread;
    uint64_t _val = _info.val;
    printf("[Finished][val:%llu][time:%.2fns/%.2fus/%.2fsec]\n", _val, _avg_time, _avg_time / 1000, _avg_time / (1000000000));
    return 0;
}