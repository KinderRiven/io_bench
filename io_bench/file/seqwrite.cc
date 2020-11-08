#include <algorithm>
#include <assert.h>
#include <dirent.h>
#include <fcntl.h>
#include <fstream>
#include <iostream>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <thread>
#include <time.h>
#include <unistd.h>
#include <vector>

#include "random.h"
#include "timer.h"

struct worker_t {
public:
    int id;
    int fd;
    uint64_t base;
    uint64_t cnt;
};

static const int kBlockSize = 1024 * 1024;

static char g_result_save_path[128];

static void result_output(const char* name, std::vector<uint64_t>& data)
{
    std::ofstream fout(name);
    if (fout.is_open()) {
        for (int i = 0; i < data.size(); i++) {
            fout << data[i] << std::endl;
        }
        fout.close();
    }
}

static void io_handle(worker_t* worker)
{
    int _id = worker->id;
    int _fd = worker->fd;
    uint64_t _cnt = worker->cnt;
    uint64_t _base = worker->base;

    Timer _t1;
    Timer _t2;
    void* _buff;
    std::vector<uint64_t> _vec_latency;
    posix_memalign(&_buff, kBlockSize, kBlockSize);
    memset(_buff, 0xff, kBlockSize);

    printf("[%d][BLOCK_SIZE:%d][TOTAL_SIZE:%zuGB]\n", _id, kBlockSize, (size_t)_cnt * kBlockSize / (1024 * 1024 * 1024));
    _t1.Start();
    for (uint64_t i = 0; i < _cnt; i++) {
        uint64_t __offset = (_base + i) * kBlockSize;
        _t2.Start();
        pwrite(_fd, _buff, kBlockSize, __offset);
        _t2.Stop();
        _vec_latency.push_back(_t2.Get());
    }
    _t1.Stop();

    double _sec = _t1.GetSeconds();
    double _bw = (1.0 * _cnt * kBlockSize / (1024 * 1024 * 1024)) / _sec;
    char _save_path[128];
    sprintf(_save_path, "%s/%d.lat", g_result_save_path, _id);
    printf("[%d][COUNT:%zu][TIME:%.2f][BW:%.2fGB/s]\n", _id, _vec_latency.size(), _sec, _bw);
    result_output(_save_path, _vec_latency);
    _vec_latency.clear();
}

// ./seqwrite [run_count] [device_mount_path] [device_capcity] [num_thread]
int main(int argc, char** argv)
{
    if (argc < 4) {
        printf("./seqwrite [run_count] [device_mount_path] [device_capcity] [num_thread]\n");
        return 1;
    }

    time_t _t = time(NULL);
    struct tm* _lt = localtime(&_t);
    sprintf(g_result_save_path, "seqwrite_%04d%02d%02d_%02d%02d%02d", _lt->tm_year, _lt->tm_mon, _lt->tm_mday, _lt->tm_hour, _lt->tm_min, _lt->tm_sec);
    mkdir(g_result_save_path, 0777);

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