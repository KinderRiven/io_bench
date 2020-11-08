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
    int fd;
    uint64_t base;
    uint64_t cnt;
};

static const int kBlockSize = 1024 * 1024;

static void io_handle(worker_t* worker)
{
    int _id = worker->id;
    int _fd = worker->fd;
    uint64_t _cnt = worker->cnt;
    uint64_t _base = worker->base;

    void* _buff;
    posix_memalign(&_buff, kBlockSize, kBlockSize);
    memset(_buff, 0xff, kBlockSize);

    printf("[%d][BLOCK_SIZE:%d][TOTAL_SIZE:%zuGB]\n", _id, kBlockSize, (size_t)_cnt * kBlockSize / (1024 * 1024 * 1024));
    for (uint64_t i = 0; i < _cnt; i++) {
        uint64_t __offset = (_base + i) * kBlockSize;
        pwrite(_fd, _buff, kBlockSize, __offset);
    }
}

// ./seqwrite [run_count] [device_mount_path] [device_capcity] [num_thread]
int main(int argc, char** argv)
{
    if (argc < 4) {
        printf("./seqwrite [run_count] [device_mount_path] [device_capcity] [num_thread]\n");
        return 1;
    }

    int _times = atol(argv[1]);
    char* _dpath = argv[2];
    size_t _size = atol(argv[3]) * (1024 * 1024 * 1024);
    int _num_thread = atol(argv[4]); // num read thread

    // create file
    char _fname[128];
    sprintf(_fname, "%s/io_bench", _dpath);
    printf("CREATE NEW FILE (%s)\n", _fname);
    int _fd = open(_fname, O_RDWR | O_DIRECT | O_CREAT, 0666);
    fallocate(_fd, 0, 0, _size);
    assert(_fd > 0);

    int _cnt = 0;
    uint64_t _min = 0;
    worker_t _workers[32];
    std::thread _threads[32];
    uint64_t _io_cnt = _size / kBlockSize;
    uint64_t _per_worker_io_cnt = _io_cnt / _num_thread;

    for (int i = 0; i < _num_thread; i++) {
        _workers[i].id = i;
        _workers[i].fd = _fd;
        _workers[i].base = _min;
        _workers[i].cnt = _per_worker_io_cnt;
        _min += _per_worker_io_cnt;
        _threads[i] = std::thread(io_handle, &_workers[i]);
    }
    for (int i = 0; i < _num_thread; i++) {
        _threads[i].join();
    }

    close(_fd);
    printf("FINISHED!\n");
    return 0;
}