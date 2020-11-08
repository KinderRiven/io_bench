#include <assert.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/syscall.h>
#include <thread>
#include <unistd.h>

#include "random.h"

struct worker_t {
public:
    int id;
    char* base;
    uint64_t io_base;
    uint64_t io_cnt;
    Random* random;
};

static const int kBlockSize = 16 * 1024;

static void io_handle(worker_t* worker)
{
    int _id = worker->id;
    uint64_t _cnt = worker->io_cnt;
    uint64_t _current = worker->io_base;
    char* _base = worker->base;

    void* _buff;
    posix_memalign(&_buff, kBlockSize, kBlockSize);
    memset(_buff, 0xff, kBlockSize);

    printf("[%d][bs:%d][size:%zuGB]\n", _id, kBlockSize, (size_t)_cnt * kBlockSize / (1024 * 1024 * 1024));
    for (uint64_t i = 0; i < _cnt; i++) {
        char* __p = _base + (_current + i) * kBlockSize;
        memcpy(__p, _buff, kBlockSize); // do write
    }
}

// ./full_write [run_count] [device_path] [device_capcity] [num_thread]
int main(int argc, char** argv)
{
    char _dname[128] = "/dev/nvme2n1";
    size_t _size = 16UL * 1024 * 1024 * 1024;
    char* _mmap_address = nullptr; // device mmap address
    int _num_thread = 4; // num read thread

    int _fd = open(_dname, O_RDWR, 0666);
    assert(_fd > 0);

    _mmap_address = (char*)mmap(nullptr, _size, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_POPULATE, _fd, 0);
    assert(_mmap_address != nullptr);

    int _cnt = 0;
    worker_t _workers[32];
    std::thread _threads[32];
    uint64_t _io_cnt = _size / kBlockSize;
    uint64_t _per_worker_io_cnt = _io_cnt / _num_thread;
    uint64_t _min = 0;

    for (int i = 0; i < _num_thread; i++) {
        _workers[i].base = _mmap_address;
        _workers[i].io_base = _min;
        _workers[i].io_cnt = _per_worker_io_cnt;
        _min += _per_worker_io_cnt;
        _threads[i] = std::thread(io_handle, &_workers[i]);
    }

    for (int i = 0; i < _num_thread; i++) {
        _threads[i].join();
    }

    munmap(_mmap_address, _size);
    close(_fd);
    printf("finished!\n");
    return 0;
}