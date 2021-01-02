#include "header.h"

struct info_t {
public:
    int a;
    int b;
    // int c;
    // int d;
    // int e;
    int finished;

public:
    info_t()
    {
        a = b = finished = 0;
    }
};

static void T1(info_t* info)
{
    cpu_set_t _mask;
    CPU_ZERO(&_mask);
    CPU_SET(0, &_mask);
    if (pthread_setaffinity_np(pthread_self(), sizeof(_mask), &_mask) < 0) {
        printf("threadpool, set thread affinity failed.\n");
    }

    while (1) {
        while (info->finished == 1) {
        }
        info->a++;
        info->b++;
        info->finished = 1;
    }
}

static void T2(info_t* info)
{
    cpu_set_t _mask;
    CPU_ZERO(&_mask);
    CPU_SET(1, &_mask);
    if (pthread_setaffinity_np(pthread_self(), sizeof(_mask), &_mask) < 0) {
        printf("threadpool, set thread affinity failed.\n");
    }

    while (1) {
        while (info->finished == 0) {
        }
        int _a = info->a;
        int _b = info->b;
        int _f = info->finished;
        if (_a != _b) {
            printf("[%d] %d %d\n", _f, _a, _b);
        }
        info->finished = 0;
    }
}

int main(int argc, char** argv)
{
    info_t _info;
    std::thread _threads[16];

    _threads[0] = std::thread(T1, &_info);
    _threads[1] = std::thread(T2, &_info);

    _threads[0].join();
    _threads[1].join();
    return 0;
}