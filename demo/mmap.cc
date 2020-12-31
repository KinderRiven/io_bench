#include "header.h"
#include "timer.h"

// #define USE_MAP_FILE

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

#ifdef USE_MAP_FILE
    int _fd = open(_name, O_RDWR | O_DIRECT, 0666);
    void* _base = mmap(nullptr, _size, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_POPULATE, _fd, 0);
#else
// case 1
#if 0
    void* _base = mmap(nullptr, _size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    printf("[base:0x%llx]\n", (uint64_t)_base);
#endif
// case 2
#if 0
    void* _base = mmap(nullptr, _size, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    printf("[base:0x%llx]\n", (uint64_t)_base);
#endif
// case 3
// OK
#if 0
    int _fd = open(_name, O_RDWR | O_DIRECT, 0666);
    void* _base1 = mmap(nullptr, _size, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    void* _base = mmap(_base1, _size, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_FIXED, _fd, 0);
    printf("[base1:0x%llu][base:0x%llx]\n", (uint64_t)_base1, (uint64_t)_base);
#endif
// case 4
// OK
#if 0
    int _fd = open(_name, O_RDWR | O_DIRECT, 0666);
    void* _base = mmap((void*)0x7fdfa8e2b000, _size, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_FIXED, _fd, 0);
    printf("[base:0x%llx]\n", (uint64_t)_base);
#endif
// case 5
#if 1
    int _fd = open(_name, O_RDWR | O_DIRECT, 0666);
    void* _base1;
    posix_memalign(&_base1, 4096, _size);
    printf("memset...\n");
    memset(_base1, 0xff, _size);
    printf("mmap...\n");
    void* _base = mmap(_base1, _size, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_FIXED, _fd, 0);
    printf("[base1:0x%llx][base:0x%llx]\n", (uint64_t)_base1, (uint64_t)_base);
#endif
#endif

    printf("[%s][size:%zu][bs:%zu][addr:0x%llx]\n", _name, _size, _block_size, (uint64_t)_base);
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

#ifdef USE_MAP_FILE
    close(_fd);
#endif
    return 0;
}