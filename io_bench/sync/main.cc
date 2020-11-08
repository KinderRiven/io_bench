#include <assert.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/syscall.h>
#include <unistd.h>

int main(int argc, char** argv)
{
    char _dname[128];
    size_t _size = 16UL * 1024 * 1024 * 1024;
    char* _base_address;

    int _fd = open(_dname, O_RDWR | O_DIRECT, 0666);
    assert(_fd > 0);

    _base_address = (char*)mmap(nullptr, _size, PROT_READ | PROT_WRITE, MAP_SHARED, _fd, 0);
    assert(_base_address != nullptr);
    return 0;
}