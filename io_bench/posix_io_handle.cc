#include "run_workload.h"

using namespace io_bench;

struct io_thread_t {
};

static io_thread_t g_io_threads[64];
static std::thread g_threads[64];

static void run_io_thread(io_thread_t* io_thread)
{

o_read:
    goto end;

o_write:
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
}

PosixIOHandle::~PosixIOHandle()
{
}

void PosixIOHandle::Run()
{
    int _thread_id = 0;

    for (int i = 0; i < options_->num_write_thread; i++, _thread_id++) {
        g_threads[_thread_id] = std::thread(run_io_thread, &g_io_threads[_thread_id]);
    }

    for (int i = 0; i < options_->num_read_thread; i++, _thread_id++) {
        g_threads[_thread_id] = std::thread(run_io_thread, &g_io_threads[_thread_id]);
    }

    _thread_id = 0;

    for (int i = 0; i < options_->num_write_thread; i++, _thread_id++) {
        g_threads[_thread_id].detach();
    }

    for (int i = 0; i < options_->num_read_thread; i++, _thread_id++) {
        g_threads[_thread_id].detach();
    }
}

void PosixIOHandle::Print()
{
}
