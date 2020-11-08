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

#define DO_READ (0)
#define DO_WRITE (1)

struct worker_t {
public:
    int id;
    int fd;
    int type; // random read/write
    uint64_t sec;
    size_t io_unit;
    uint64_t file_size;
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
    int _type = worker->type;
    int _id = worker->id;
    int _fd = worker->fd;
    size_t _io_unit = worker->io_unit;
    uint64_t _maxv = worker->file_size / _io_unit;
    uint64_t _time = worker->sec * 1000000000UL;
    Random* _random = worker->random;

    Timer _t1;
    Timer _t2;
    void* _buff;
    std::vector<uint64_t> _vec_latency;
    uint64_t _sum_lat = 0;
    posix_memalign(&_buff, 4096, _io_unit);
    memset(_buff, 0xff, _io_unit);

    printf("[%d][RANGE:(0,%llu)][IO_SIZE:%zu]\n", _id, _maxv, _io_unit);
    _t1.Start();
    for (;;) {
        uint32_t __s = _random->Next() % _maxv;
        uint64_t __offset = __s * _io_unit;
        _t2.Start();
        if (_type == DO_READ) {
            pread(_fd, _buff, _io_unit, __offset);
        } else {
            pwrite(_fd, _buff, _io_unit, __offset);
        }
        _t2.Stop();
        _sum_lat += _t2.Get();
        _vec_latency.push_back(_t2.Get());
        _t1.Stop();
        if (_t1.Get() > _time) {
            break;
        }
    }
    _t1.Stop();

    double _sec = _t1.GetSeconds();
    double _lat = 1.0 * _sum_lat / _vec_latency.size();
    _lat /= 1000; // ns->us
    char _save_path[128];
    if (_type == DO_READ) {
        sprintf(_save_path, "%s/read_%d.lat", g_result_save_path, _id);
    } else {
        sprintf(_save_path, "%s/write_%d.lat", g_result_save_path, _id);
    }
    printf("[%d][COUNT:%zu][TIME:%.2f][Lat:%.2fus]\n", _id, _vec_latency.size(), _sec, _lat);
    result_output(_save_path, _vec_latency);
    _vec_latency.clear();
}

// ./randwrite [device_mount_path] [device_capcity] [num_thread] [block_size(B)] [time(seconds)]
int main(int argc, char** argv)
{
    if (argc < 7) {
        printf("./readwrite [device_mount_path] [device_capcity|GB] [n_rthread] [r_bs|B] [n_wthread] [w_bs|B] [time|sec]\n");
        return 1;
    }

    char* _dpath = argv[1];
    size_t _size = atol(argv[2]) * (1024 * 1024 * 1024);
    int _num_rthread = atol(argv[3]); // num read thread
    int _rbs = atol(argv[4]);
    int _num_wthread = atol(argv[5]);
    int _wbs = atol(argv[6]);
    uint64_t _sec = atol(argv[7]);

    time_t _t = time(NULL);
    struct tm* _lt = localtime(&_t);
    sprintf(g_result_save_path, "rw_%d_%d_%04d%02d%02d_%02d%02d%02d", _rbs, _wbs, _lt->tm_year, _lt->tm_mon, _lt->tm_mday, _lt->tm_hour, _lt->tm_min, _lt->tm_sec);
    mkdir(g_result_save_path, 0666);

    // create file
    char _fname[128];
    sprintf(_fname, "%s/io_bench", _dpath);
    printf("OPEN FILE (%s)\n", _fname);
    int _fd = open(_fname, O_RDWR | O_DIRECT, 0666);
    assert(_fd > 0);

    worker_t _workers[32];
    int _wcnt = 0;
    std::thread _threads[32];

    printf("[read:%d|%dB][write:%d|%dB]\n", _num_rthread, _rbs, _num_wthread, _wbs);
    for (int i = 0; i < _num_rthread; i++) {
        _workers[_wcnt].id = _wcnt;
        _workers[_wcnt].fd = _fd;
        _workers[_wcnt].type = DO_READ;
        _workers[_wcnt].sec = _sec;
        _workers[_wcnt].io_unit = _rbs;
        _workers[_wcnt].file_size = _size;
        _workers[_wcnt].random = new Random(1000 + _wcnt);
        _threads[_wcnt] = std::thread(io_handle, &_workers[_wcnt]);
        _wcnt++;
    }
    for (int i = 0; i < _num_wthread; i++) {
        _workers[_wcnt].id = _wcnt;
        _workers[_wcnt].fd = _fd;
        _workers[_wcnt].type = DO_WRITE;
        _workers[_wcnt].sec = _sec;
        _workers[_wcnt].io_unit = _rbs;
        _workers[_wcnt].file_size = _size;
        _workers[_wcnt].random = new Random(1000 + _wcnt);
        _threads[_wcnt] = std::thread(io_handle, &_workers[_wcnt]);
        _wcnt++;
    }
    for (int i = 0; i < _wcnt; i++) {
        _threads[i].join();
    }

    close(_fd);
    printf("FINISHED!\n");
    return 0;
}