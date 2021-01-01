#include "header.h"
#include "timer.h"

#define PER_IO (16)

void do_readv(int fd, size_t size, size_t block_size)
{
    void* _buff[PER_IO];
    struct iovec _iovec[PER_IO];
    size_t _cnt = (size / block_size) / PER_IO;

    for (int i = 0; i < PER_IO; i++) {
        posix_memalign(&_buff[i], 4096, block_size);
        memset(_buff[i], 0xff, block_size);
        _iovec[i].iov_base = _buff[i];
        _iovec[i].iov_len = block_size;
    }

    for (int i = 0; i < _cnt; i++) {
        readv(fd, &_iovec[0], PER_IO);
    }
}

void do_writev(int fd, size_t size, size_t block_size)
{
    void* _buff[PER_IO];
    struct iovec _iovec[PER_IO];
    size_t _cnt = (size / block_size) / PER_IO;

    for (int i = 0; i < PER_IO; i++) {
        posix_memalign(&_buff[i], 4096, block_size);
        memset(_buff[i], 0xff, block_size);
        _iovec[i].iov_base = _buff[i];
        _iovec[i].iov_len = block_size;
    }

    for (int i = 0; i < _cnt; i++) {
        writev(fd, &_iovec[0], PER_IO);
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
    do_readv(_fd, _size, _block_size);
    _timer.Stop();
    printf("read time:%.2fseconds\n", 1.0 * _timer.Get() / 1000000000);
#endif

#if 0
    _timer.Start();
    do_writev(_fd, _size, _block_size);
    _timer.Stop();
    printf("write time:%.2fseconds\n", 1.0 * _timer.Get() / 1000000000);
#endif
    close(_fd);
    return 0;
}