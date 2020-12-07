#include "run_workload.h"
#include "timer.h"

using namespace io_bench;

struct io_thread_t {
public:
    // DDDD
    int thread_id;

    // 0是读，1是写
    int rw;

    // IO类型：随机或顺序
    io_type_t io_type;

    // 进行io的文件描述符
    int fd;

    // 在该文件的哪一段范围进行IO操作
    uint64_t io_base;

    uint64_t io_space_size;

    // 一共要进行的IO量
    size_t io_total_size;

    // IO的粒度
    size_t io_block_size;

public:
    // 记录每个请求的延迟
    std::vector<uint64_t> vec_latency;

    // 所有的请求延迟和
    uint64_t total_time;

    // 平均时间
    double avg_time;
};

static volatile int g_stop = 0;

static io_thread_t g_io_threads[64];

static std::thread g_threads[64];

static void run_io_thread_based_size(io_thread_t* io_thread)
{
    Timer _timer;
    int _fd = io_thread->fd;
    size_t _io_total_size = io_thread->io_total_size;
    size_t _io_block_size = io_thread->io_block_size;
    uint64_t _do_count = _io_total_size / _io_block_size;

    // 为了测试效果最好，需要进行4KB的对齐
    uint64_t _io_start = ((io_thread->io_base + 4096) & (~(uint64_t)4095));
    uint64_t _io_end = _io_start + io_thread->io_space_size;
    uint64_t _pos = _io_start;

    // DIRECT_IO需要512B的对齐，为了测试效果最好，进行4KB的内存申请
    void* _buff;
    posix_memalign(&_buff, 4096, _io_block_size);
    memset(_buff, 0xff, _io_block_size);
    assert(_io_start % 4096 == 0);

    printf("[thread:%02d][fd:%d][start:%lluMB][end:%lluMB][BS:%zuB][SIZE:%zuMB][COUNT:%llu]\n",
        io_thread->thread_id, _fd, _io_start / (1024 * 1024), _io_end / (1024 * 1024),
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

do_seq_read:
    for (int i = 0; i < _do_count; i++) {
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
    }
    goto end;

do_seq_write:
    for (int i = 0; i < _do_count; i++) {
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
    }
    goto end;

do_random_read:
    goto end;

do_random_write:
    goto end;

end:
    io_thread->avg_time = 1.0 * io_thread->total_time / _do_count;
    printf("[thread:%02d][total_time:%.2fseconds][avg_time:%.2fus]\n",
        io_thread->thread_id, 1.0 * io_thread->total_time / (1000000000UL), io_thread->avg_time / 1000);
    return;
}

static void run_io_thread_based_time(io_thread_t* io_thread)
{
do_seq_read:
    goto end;

do_seq_write:
    goto end;

do_random_read:
    goto end;

do_random_write:
    goto end;

end:
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
        g_io_threads[_thread_id].thread_id = _thread_id;
        g_io_threads[_thread_id].fd = fd_[_thread_id];
        g_io_threads[_thread_id].rw = 1;
        g_io_threads[_thread_id].io_base = 0; // _thread_id * _per_thread_io_space_size;
        g_io_threads[_thread_id].io_space_size = _per_thread_io_space_size;
        g_io_threads[_thread_id].io_total_size = _per_thread_io_size;
        g_io_threads[_thread_id].io_block_size = options_->block_size;
        if (options_->time_based) {
            g_threads[_thread_id] = std::thread(run_io_thread_based_time, &g_io_threads[_thread_id]);
        } else {
            g_threads[_thread_id] = std::thread(run_io_thread_based_size, &g_io_threads[_thread_id]);
        }
    }

    for (int i = 0; i < options_->num_read_thread; i++, _thread_id++) {
        g_io_threads[_thread_id].thread_id = _thread_id;
        g_io_threads[_thread_id].fd = fd_[_thread_id];
        g_io_threads[_thread_id].rw = 0;
        g_io_threads[_thread_id].io_base = 0; // _thread_id * _per_thread_io_space_size;
        g_io_threads[_thread_id].io_space_size = _per_thread_io_space_size;
        g_io_threads[_thread_id].io_total_size = _per_thread_io_size;
        g_io_threads[_thread_id].io_block_size = options_->block_size;
        if (options_->time_based) {
            g_threads[_thread_id] = std::thread(run_io_thread_based_time, &g_io_threads[_thread_id]);
        } else {
            g_threads[_thread_id] = std::thread(run_io_thread_based_size, &g_io_threads[_thread_id]);
        }
    }

    _thread_id = 0;

    for (int i = 0; i < options_->num_write_thread; i++, _thread_id++) {
        g_threads[_thread_id].join();
    }

    for (int i = 0; i < options_->num_read_thread; i++, _thread_id++) {
        g_threads[_thread_id].join();
    }
}

void PosixIOHandle::Print()
{
}
