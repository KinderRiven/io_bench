#include "generate_workload.h"
#include "header.h"
#include "run_workload.h"

enum workload_type_t {
    WORKLOAD_DBBENCH,
    WORKLOAD_YCSB,
};

enum io_handle_type_t {
    IO_HANDLE_POSIX,
    IO_HANDLE_SPDK,
};

// 是否需要warmup操作，如果为ture，则进行实际测试前会对要做IO的空间做一遍顺序写
static bool g_warmup = true;

// 负载的生成类型
static workload_type_t g_workload_type = WORKLOAD_DBBENCH;

// IO的处理方式
// [1] posix：通过文件系统read/write接口进行数据读写
// [2] mmap：通过mmap+memcpy进行数据读写
// [3] libaio：Linux AIO接口
// [4] spdk：通过SPDK的用户态接口进行数据读写
static io_handle_type_t g_io_handle_type = IO_HANDLE_POSIX;

static io_bench::IO_Options g_options;

void do_parse_parameters(int argc, char** argv)
{
    for (int i = 0; i < argc; i++) {
        double d;
        uint64_t n;
        char junk;
        if (sscanf(argv[i], "--io_size=%llu%c", &n, &junk) == 1) {
            g_options.io_size = n;
        } else if (sscanf(argv[i], "--space_size=%llu%c", &n, &junk) == 1) {
            g_options.space_size = n;
        } else if (sscanf(argv[i], "--time=%llu%c", &n, &junk) == 1) {
            g_options.time_based = true;
            g_options.time = n;
        } else if (sscanf(argv[i], "--num_write_thread=%llu%c", &n, &junk) == 1) {
            g_options.num_write_thread = n;
        } else if (sscanf(argv[i], "--num_read_thread=%llu%c", &n, &junk) == 1) {
            g_options.num_read_thread = n;
        } else if (strncmp(argv[i], "--path=", 7) == 0) {
            strcpy(g_options.path, argv[i] + 7);
        } else if (strncmp(argv[i], "--name=", 7) == 0) {
            strcpy(g_options.name, argv[i] + 7);
        } else if (strncmp(argv[i], "--warm", 6) == 0) {
            g_warmup = true;
        } else if (i > 0) {
            exit(1);
        }
    }
}

int main(int argc, char** argv)
{
    do_parse_parameters(argc, argv);

    if (g_io_handle_type == IO_HANDLE_POSIX) {
        // 如果需要warmup，则在正式测试前开一个warmup的负载
        if (g_warmup == true) {
            io_bench::IO_Options _warm_opt = g_options;
            _warm_opt.num_write_thread += _warm_opt.num_read_thread;
            _warm_opt.num_read_thread = 0;
            _warm_opt.write_type = io_bench::IO_SEQ; // warmup的时候开多线程进行顺序写
            io_bench::IOHandle* _warmup_io_handle = new io_bench::PosixIOHandle(&_warm_opt);
            _warmup_io_handle->Run();
            _warmup_io_handle->Print();
        }

        // 开始进行IO测试
        io_bench::IOHandle* _test_io_handle = new io_bench::PosixIOHandle(&g_options);
        _test_io_handle->Run();
        _test_io_handle->Print();
    }
    return 0;
}