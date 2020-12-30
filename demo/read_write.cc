#include "header.h"
#include "timer.h"

void do_read(int fd, size_t size, size_t block_size)
{
    void* _buff = nullptr;
    size_t _cnt = size / block_size;

    posix_memalign(&_buff, 4096, block_size);
    memset(_buff, 0xff, block_size);

    char* _dest = (char*)_buff;
    uint64_t _pos = 0;

    for (int i = 0; i < _cnt; i++) {
        pread(fd, _dest, block_size, _pos);
        _pos += block_size;
    }
}

void do_write(int fd, size_t size, size_t block_size)
{
    void* _buff = nullptr;
    size_t _cnt = size / block_size;

    posix_memalign(&_buff, 4096, block_size);
    memset(_buff, 0xff, block_size);

    uint64_t _pos = 0;
    char* _src = (char*)_buff;

    for (int i = 0; i < _cnt; i++) {
        pwrite(fd, _src, block_size, _pos);
        _pos += block_size;
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
    assert(_fd > 0);

#if 1
    _timer.Start();
    do_read(_fd, _size, _block_size);
    _timer.Stop();
    printf("read time:%.2fseconds\n", 1.0 * _timer.Get() / 1000000000);
#endif

#if 1
    _timer.Start();
    do_write(_fd, _size, _block_size);
    _timer.Stop();
    printf("write time:%.2fseconds\n", 1.0 * _timer.Get() / 1000000000);
#endif
    close(_fd);
    return 0;
}