#include "run_workload.h"
#include "timer.h"

using namespace io_bench;

static volatile int g_stop = 0;

static void run_io_thread(io_thread_t* io_thread)
{
    bool _time_based = io_thread->time_based;
    int _run_time = io_thread->time * 1000000000UL;
    Timer _timer;
    int _fd = io_thread->fd;
    Workload* _workload = io_thread->workload;

    size_t _io_space_size = io_thread->io_space_size;
    size_t _io_total_size = io_thread->io_total_size;
    size_t _io_block_size = io_thread->io_block_size;

    uint64_t _space_count = _io_space_size / _io_block_size;
    uint64_t _do_count = (_time_based == true) ? 0 : _io_total_size / _io_block_size;

    // 为了测试效果最好，需要进行4KB的对齐
    uint64_t _io_start = 0;
    uint64_t _io_end = _io_start + io_thread->io_space_size;
    uint64_t _pos = _io_start;

    // DIRECT_IO需要512B的对齐，为了测试效果最好，进行4KB的内存申请
    void* _buff;
    posix_memalign(&_buff, 4096, _io_block_size);
    memset(_buff, 0xff, _io_block_size);
    assert(_io_start % 4096 == 0);

    printf("[thread:%02d][fd:%d][start:%lluMB][end:%lluMB][SC:%llu][BS:%zuB][SIZE:%zuMB][COUNT:%llu]\n",
        io_thread->thread_id, _fd, _io_start / (1024 * 1024), _io_end / (1024 * 1024), _space_count,
        _io_block_size, _io_total_size / (1024 * 1024), _do_count);

    if (io_thread->rw == 1) {
        if (io_thread->io_type == IO_SEQ) {
            goto do_seq_write;
        } else {
            goto do_random_write;
        }
    } else {
        if (io_thread->io_type == IO_SEQ) {
            goto do_seq_read;
        } else {
            goto do_random_read;
        }
    }

do_seq_read: // 顺序读开始
    printf("[thread:%02d][do_seq_read]\n", io_thread->thread_id);
    for (int i = 0;; i++) {
        _timer.Start();
        pread(_fd, _buff, _io_block_size, _pos);
        _timer.Stop();

        _pos += _io_block_size;
        if (_pos > _io_end) {
            _pos = _io_start;
        }
        uint64_t _t = _timer.Get();
        io_thread->vec_latency.push_back(_t);
        io_thread->total_time += _t;

        // 判断结束方式
        if (_time_based) {
            if (io_thread->total_time > _run_time) {
                break;
            }
            _do_count++;
        } else if (i > _do_count) {
            break;
        }
    }
    goto end; // 顺序读结束

do_seq_write: // 顺序写开始
    printf("[thread:%02d][do_seq_write]\n", io_thread->thread_id);
    for (int i = 0;; i++) {
        _timer.Start();
        pwrite(_fd, _buff, _io_block_size, _pos);
        _timer.Stop();

        _pos += _io_block_size;
        if (_pos > _io_end) {
            _pos = _io_start;
        }
        uint64_t _t = _timer.Get();
        io_thread->vec_latency.push_back(_t);
        io_thread->total_time += _t;

        // 判断结束方式
        if (_time_based) {
            if (io_thread->total_time > _run_time) {
                break;
            }
            _do_count++;
        } else if (i > _do_count) {
            break;
        }
    }
    goto end; // 顺序写结束

do_random_read: // 随机读开始
    printf("[thread:%02d][do_seq_read]\n", io_thread->thread_id);
    for (int i = 0;; i++) {
        _pos = (_workload->Get() % _space_count) * io_thread->io_block_size;
        _timer.Start();
        pread(_fd, _buff, _io_block_size, _pos);
        _timer.Stop();

        uint64_t _t = _timer.Get();
        io_thread->vec_latency.push_back(_t);
        io_thread->total_time += _t;

        // 判断结束方式
        if (_time_based) {
            if (io_thread->total_time > _run_time) {
                break;
            }
            _do_count++;
        } else if (i > _do_count) {
            break;
        }
    }
    goto end; // 随机读结束

do_random_write: // 随机写开始
    printf("[thread:%02d][do_random_write]\n", io_thread->thread_id);
    for (int i = 0;; i++) {
        _pos = (_workload->Get() % _space_count) * io_thread->io_block_size;
        _timer.Start();
        pwrite(_fd, _buff, _io_block_size, _pos);
        _timer.Stop();

        uint64_t _t = _timer.Get();
        io_thread->vec_latency.push_back(_t);
        io_thread->total_time += _t;

        // 判断结束方式
        if (_time_based) {
            if (io_thread->total_time > _run_time) {
                break;
            }
            _do_count++;
        } else if (i > _do_count) {
            break;
        }
    }
    goto end; // 随机写结束

end:
    io_thread->avg_time = 1.0 * io_thread->total_time / _do_count;
    io_thread->iops = 1000000000.0 / io_thread->avg_time;
    printf("[thread:%02d][count:%llu][total_time:%.2fseconds][avg_time:%.2fus][iops:%.2f]\n",
        io_thread->thread_id, _do_count, 1.0 * io_thread->total_time / (1000000000UL), io_thread->avg_time / 1000, io_thread->iops);
    return;
}

PosixIOHandle::PosixIOHandle(IO_Options* options)
    : options_(options)
{
    // 建立结果输出文件夹
    time_t _t = time(NULL);
    struct tm* _lt = localtime(&_t);
    sprintf(result_save_path_, "%04d%02d%02d_%02d%02d%02d", _lt->tm_year, _lt->tm_mon, _lt->tm_mday, _lt->tm_hour, _lt->tm_min, _lt->tm_sec);
    mkdir(result_save_path_, 0666);

    // 建立一个大文件
    size_t _per_thread_io_space_size = options_->space_size / (options_->num_write_thread + options_->num_read_thread);
    sprintf(file_path_, "%s/%s", options->path, options->name);

    for (int i = 0; i < (options->num_read_thread + options->num_write_thread); i++) {
        char _new_file[128];
        sprintf(_new_file, "%s.%d", file_path_, i);
        fd_[i] = open(_new_file, O_RDWR | O_DIRECT | O_CREAT, 0666);
        printf("CREATE FILE (%s)\n", _new_file);
        // 打洞，DDDDD
        fallocate(fd_[i], 0, 0, _per_thread_io_space_size);
    }
}

PosixIOHandle::~PosixIOHandle()
{
}

void PosixIOHandle::Run()
{
    int _thread_id = 0;
    size_t _per_thread_io_size = options_->io_size / (options_->num_write_thread + options_->num_read_thread);
    size_t _per_thread_io_space_size = options_->space_size / (options_->num_write_thread + options_->num_read_thread);

    for (int i = 0; i < options_->num_write_thread; i++, _thread_id++) {
        // 传参
        io_threads_[_thread_id].thread_id = _thread_id;
        io_threads_[_thread_id].fd = fd_[_thread_id];
        io_threads_[_thread_id].rw = 1;
        io_threads_[_thread_id].io_type = options_->write_type;
        io_threads_[_thread_id].io_space_size = _per_thread_io_space_size;
        io_threads_[_thread_id].io_total_size = _per_thread_io_size;
        io_threads_[_thread_id].io_block_size = options_->block_size;

        // Time based
        if (options_->time_based) {
            io_threads_[_thread_id].time_based = true;
            io_threads_[_thread_id].time = options_->time;
        }

        // 创建负载
        if (options_->workload_type == WORKLOAD_DBBENCH) {
            io_threads_[_thread_id].workload = new DBBenchWorkload(1000 + _thread_id);
        } else if (options_->workload_type == WORKLOAD_YCSB) {
            io_threads_[_thread_id].workload = new YCSBWorkload();
        }

        // 创建线程
        threads_[_thread_id] = std::thread(run_io_thread, &io_threads_[_thread_id]);
    }

    for (int i = 0; i < options_->num_read_thread; i++, _thread_id++) {
        // 传参
        io_threads_[_thread_id].thread_id = _thread_id;
        io_threads_[_thread_id].fd = fd_[_thread_id];
        io_threads_[_thread_id].rw = 0;
        io_threads_[_thread_id].io_type = options_->read_type;
        io_threads_[_thread_id].io_space_size = _per_thread_io_space_size;
        io_threads_[_thread_id].io_total_size = _per_thread_io_size;
        io_threads_[_thread_id].io_block_size = options_->block_size;

        // Time based
        if (options_->time_based) {
            io_threads_[_thread_id].time_based = true;
            io_threads_[_thread_id].time = options_->time;
        }

        // 创建负载
        if (options_->workload_type == WORKLOAD_DBBENCH) {
            io_threads_[_thread_id].workload = new DBBenchWorkload(1000 + _thread_id);
        } else if (options_->workload_type == WORKLOAD_YCSB) {
            io_threads_[_thread_id].workload = new YCSBWorkload();
        }

        // 创建线程
        threads_[_thread_id] = std::thread(run_io_thread, &io_threads_[_thread_id]);
    }

    _thread_id = 0;
    for (int i = 0; i < options_->num_write_thread; i++, _thread_id++) {
        threads_[_thread_id].join();
        close(io_threads_[_thread_id].fd);
    }
    for (int i = 0; i < options_->num_read_thread; i++, _thread_id++) {
        threads_[_thread_id].join();
        close(io_threads_[_thread_id].fd);
    }
}

void PosixIOHandle::Print()
{
}
