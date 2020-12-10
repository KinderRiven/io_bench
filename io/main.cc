#include "generate_workload.h"
#include "header.h"
#include "run_workload.h"

enum io_handle_type_t {
    IO_HANDLE_POSIX,
    IO_HANDLE_SPDK,
};

// 是否需要warmup操作，如果为ture，则进行实际测试前会对要做IO的空间做一遍顺序写
static bool g_warmup = true;

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
            g_options.io_size = n * (1024UL * 1024 * 1024);
        } else if (sscanf(argv[i], "--space_size=%llu%c", &n, &junk) == 1) {
            g_options.space_size = n * (1024UL * 1024 * 1024);
        } else if (sscanf(argv[i], "--time=%llu%c", &n, &junk) == 1) {
            g_options.time_based = true;
            g_options.time = n;
        } else if (sscanf(argv[i], "--block_size=%llu%c", &n, &junk) == 1) {
            g_options.block_size = n;
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
        } else if (strncmp(argv[i], "--read=", 7) == 0) {
            if (strcmp(argv[i] + 7, "seq") == 0) {
                g_options.read_type = io_bench::IO_SEQ;
            } else if (strcmp(argv[i] + 7, "random") == 0) {
                g_options.read_type = io_bench::IO_RANDOM;
            } else {
                goto bad;
            }
        } else if (strncmp(argv[i], "--write=", 8) == 0) {
            if (strcmp(argv[i] + 8, "seq") == 0) {
                g_options.write_type = io_bench::IO_SEQ;
            } else if (strcmp(argv[i] + 8, "random") == 0) {
                g_options.write_type = io_bench::IO_RANDOM;
            } else {
                goto bad;
            }
        } else if (i > 0) {
        bad:
            printf("Something Bad!\n");
            exit(1);
        }
    }
}

int main(int argc, char** argv)
{
    do_parse_parameters(argc, argv);
    printf("Parse Parameteers Finished!\n");

    struct spdk_env_opts _env_opts;
    spdk_env_opts_init(&_env_opts);
    spdk_env_init(&_env_opts);

    if (g_io_handle_type == IO_HANDLE_POSIX) {
        // 如果需要warmup，则在正式测试前开一个warmup的负载
        if (g_warmup == true) {
            printf("Warmup!\n");
            io_bench::IO_Options _warm_opt = g_options;
            _warm_opt.num_write_thread += _warm_opt.num_read_thread;
            _warm_opt.num_read_thread = 0;
            _warm_opt.write_type = io_bench::IO_SEQ; // warmup的时候开多线程进行顺序写
            // io_bench::IOHandle* _warmup_io_handle = new io_bench::PosixIOHandle(&_warm_opt);
            io_bench::IOHandle* _warmup_io_handle = new io_bench::SpdkIOHandle(&_warm_opt);
            _warmup_io_handle->Run();
            // _warmup_io_handle->Print();
            printf("Warmup Finished!\n");
            delete _warmup_io_handle;
            printf("Delete finished!\n");
        }
        // 开始进行IO测试
        printf("Test!\n");
        // io_bench::IOHandle* _test_io_handle = new io_bench::PosixIOHandle(&g_options);
        io_bench::IOHandle* _test_io_handle = new io_bench::SpdkIOHandle(&g_options);
        _test_io_handle->Run();
        _test_io_handle->Print();
        printf("Test Finished!\n");
        delete _test_io_handle;
    }
    return 0;
}