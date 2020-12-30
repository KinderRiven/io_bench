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
        msync(_dest, block_size, MS_SYNC);
        _dest += block_size;
    }
}

int main(int argc, char** argv)
{
    Timer _timer;
    int _scan;
    char* _name = argv[1];
    size_t _size = atol(argv[2]) * 1024 * 1024;
    size_t _block_size = atol(argv[3]);

    printf("[%s][size:%zu][bs:%zu]\n", _name, _size, _block_size);

    int _fd = open(_name, O_RDWR | O_DIRECT, 0666);
    void* _base = mmap(nullptr, _size, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_POPULATE, _fd, 0);

#if 1
    _timer.Start();
    do_read(_base, _size, _block_size);
    _timer.Stop();
    printf("read time:%.2fseconds\n", 1.0 * _timer.Get() / 1000000000);
#endif

#if 0
    _timer.Start();
    do_write(_base, _size, _block_size);
    _timer.Stop();
    printf("write time:%.2fseconds\n", 1.0 * _timer.Get() / 1000000000);
#endif
    close(_fd);
    return 0;
}