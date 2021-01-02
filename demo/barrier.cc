#include "header.h"

struct info_t {
public:
    int a;
    int b;
    int c;
    int d;
    int e;
    int finished;

public:
    info_t()
    {
        a = b = c = d = e = finished = 0;
    }
};

static void T1(info_t* info)
{
    info->a = 1;
    info->b = 2;
    info->c = 3;
    info->d = 4;
    info->e = 5;
    info->finished = 1;
}

static void T2(info_t* info)
{
    while (info->finished == 0) {
    }

    int _a = info->a;
    int _b = info->b;
    int _c = info->c;
    int _d = info->d;
    int _e = info->e;
    printf("%d %d %d %d %d\n", _a, _b, _c, _d, _e);
}

int main(int argc, char** argv)
{
    info_t _info;
    std::thread _threads[16];

    _threads[0] = std::thread(T1, &_info);
    _threads[1] = std::thread(T1, &_info);

    _threads[0].join();
    _threads[1].join();
    return 0;
}