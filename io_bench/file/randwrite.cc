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
    size_t bs;
    uint64_t cnt;
    uint64_t maxv;
    Random* random;
};

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
    uint64_t _maxv = worker->maxv;
    size_t _bs = worker->bs;
    Random* _random = worker->random;

    Timer _t1;
    Timer _t2;
    void* _buff;
    std::vector<uint64_t> _vec_latency;
    uint64_t _sum_lat = 0;
    posix_memalign(&_buff, _bs, _bs);
    memset(_buff, 0xff, _bs);

    _t1.Start();
    for (uint64_t i = 0; i < _cnt; i++) {
        uint32_t __s = _random->Next() % _maxv;
        uint64_t __offset = __s * _bs;
        _t2.Start();
        pwrite(_fd, _buff, _bs, __offset);
        _t2.Stop();
        _sum_lat += _t2.Get();
        _vec_latency.push_back(_t2.Get());
    }
    _t1.Stop();

    double _sec = _t1.GetSeconds();
    double _lat = 1.0 * _sum_lat / _vec_latency.size();
    _lat /= 1000; // ns->us
    char _save_path[128];
    sprintf(_save_path, "%s/%d.lat", g_result_save_path, _id);
    printf("[%d][COUNT:%zu][TIME:%.2f][Lat:%.2fus]\n", _id, _vec_latency.size(), _sec, _lat);
    result_output(_save_path, _vec_latency);
    _vec_latency.clear();
}

// ./seqwrite [count(GB)] [device_mount_path] [device_capcity] [num_thread]
int main(int argc, char** argv)
{
    if (argc < 4) {
        printf("./seqwrite [device_mount_path] [device_capcity] [num_thread] [block_size(B)] [count(GB)]\n");
        return 1;
    }

    char* _dpath = argv[1];
    size_t _size = atol(argv[2]) * (1024 * 1024 * 1024);
    int _num_thread = atol(argv[3]); // num read thread
    int _bs = atol(argv[4]);
    uint64_t _cnt = (uint64_t)atol(argv[5]) * (1024 * 1024 * 1024) / _bs;

    time_t _t = time(NULL);
    struct tm* _lt = localtime(&_t);
    sprintf(g_result_save_path, "randwrite%dB_%04d%02d%02d_%02d%02d%02d", _bs, _lt->tm_year, _lt->tm_mon, _lt->tm_mday, _lt->tm_hour, _lt->tm_min, _lt->tm_sec);
    mkdir(g_result_save_path, 0666);

    // create file
    char _fname[128];
    sprintf(_fname, "%s/io_bench", _dpath);
    printf("CREATE NEW FILE (%s)\n", _fname);
    int _fd = open(_fname, O_RDWR | O_DIRECT, 0666);
    assert(_fd > 0);

    worker_t _workers[32];
    std::thread _threads[32];
    uint64_t _io_cnt = _size / _bs;

    for (int i = 0; i < _num_thread; i++) {
        _workers[i].id = i;
        _workers[i].fd = _fd;
        _workers[i].maxv = _io_cnt;
        _workers[i].bs = _bs;
        _workers[i].cnt = _cnt / _num_thread;
        _workers[i].random = new Random(1000 + i);
        _threads[i] = std::thread(io_handle, &_workers[i]);
    }
    for (int i = 0; i < _num_thread; i++) {
        _threads[i].join();
    }

    close(_fd);
    printf("FINISHED!\n");
    return 0;
}