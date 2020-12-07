#ifndef INCLUDE_RUN_WORKLOAD_H_
#define INCLUDE_RUN_WORKLOAD_H_

#include "header.h"

namespace io_bench {

enum io_type_t {
    IO_SEQ,
    IO_RANDOM
};

class IO_Options {
public:
    // 是否采用DIRECT_IO的方式读写文件
    bool direct_io;

    // 设备挂载路径或SPDK初始化时的设备标识符
    char path[128];

    // IO时创建的文件名称
    char name[128];

    // IO时的范围大小（比如调用Posix IO时创建的初试文件大小）
    size_t space_size;

    // 读线程个数
    int num_read_thread;

    // 写线程个数
    int num_write_thread;

    // 读类型（随机或顺序）
    io_type_t read_type;

    // 写类型（随机或顺序）
    io_type_t write_type;

    // 本次测试是否基于时间，若此参数为true，则 io_size参数失效
    bool time_based;

    // 仅仅在 time_based为true时有效，IO运行时间
    uint64_t time;

    // 测试粒度的大小
    size_t block_size;

    // 仅仅在 time_based为false时有效，一共要做的IO_SIZE
    // 在多线程情况下，每个线程均分该IO_SIZE
    // 比如IO_SIZE为2GB，有4个写线程，则每个线程进行512MB的IO总量
    size_t io_size;

public:
    IO_Options()
        : path("/home/hanshukai/dir1")
        , name("io_bench")
    {
        read_type = IO_SEQ;

        write_type = IO_SEQ;

        direct_io = true;

        space_size = 8UL * 1024 * 1024 * 1024;

        num_read_thread = 0;

        num_write_thread = 4;

        time_based = false;

        time = 0;

        block_size = 4096;

        io_size = 8UL * 1024 * 1024 * 1024;
    }
};

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

class IOHandle {
public:
    virtual void Run() = 0;

    virtual void Print() = 0;
};

class PosixIOHandle : public IOHandle {
public:
    PosixIOHandle(IO_Options* options);

    ~PosixIOHandle();

public:
    void Run();

    void Print();

private:
    IO_Options* options_;

    char result_save_path_[128];

    char file_path_[128];

    int fd_[128];

    io_thread_t io_threads_[64];

    std::thread threads_[64];
};

class SpdkIOHandle : public IOHandle {
public:
    SpdkIOHandle(IO_Options* options);

    ~SpdkIOHandle();

public:
    void Run();

    void Print();
};
};

#endif