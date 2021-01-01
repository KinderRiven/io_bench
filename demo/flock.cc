#include "header.h"
#include "timer.h"

void do_flock(int fd)
{
    int _res;
    _res = flock(fd, LOCK_EX);
    printf("%d\n", _res);
    _res = flock(Fd, LOCK_UN);
    printf("%d\n", _res);
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
    do_flock(_fd);
    close(_fd);
    return 0;
}