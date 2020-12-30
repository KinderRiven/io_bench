#include "header.h"
#include "timer.h"

void do_read(void* addr, size_t size, size_t block_size)
{
    void* _buff = nullptr;
    size_t _cnt = size / block_size;

    posix_memalign(&_buff, 4096, block_size);
    memset(_buff, 0xff, block_size);

    char* _dest = (char*)_buff;
    char* _src = (char*)addr;

    for (int i = 0; i < _cnt; i++) {
        memcpy(_dest, _src, block_size);
        _dest += block_size;
        _src += block_size;
    }
}

void do_write(void* addr, size_t size, size_t block_size)
{
    void* _buff = nullptr;
    size_t _cnt = size / block_size;

    posix_memalign(&_buff, 4096, block_size);
    memset(_buff, 0xff, block_size);

    char* _dest = (char*)addr;
    char* _src = (char*)_buff;

    for (int i = 0; i < _cnt; i++) {
        memcpy(_dest, _src, block_size);
        _dest += block_size;
        _src += block_size;
    }
}

int main(int argc, char** argv)
{
    Timer _timer;
    char _name[128];
    size_t _size;
    int _scan;
    int _fd = open(_name, O_RDWR | O_DIRECT, 0666);
    void* _base = mmap(nullptr, _size, PROT_READ | PROT_WRITE, MAP_SHARED, _fd, 0);

    scanf("%d", &_scan);
    _timer.Start();
    do_write(_base, _size, 4096);
    _timer.Stop();
    printf("time:%.2fus\n", 1.0 * _timer.Get() / 1000000);

    scanf("%d", &_scan);
    _timer.Start();
    do_read(_base, _size, 4096);
    _timer.Stop();
    printf("time:%.2fus\n", 1.0 * _timer.Get() / 1000000);

    close(_fd);
    return 0;
}