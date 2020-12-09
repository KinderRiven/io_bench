#ifndef INCLUDE_RUN_WORKLOAD_H_
#define INCLUDE_RUN_WORKLOAD_H_

#include "generate_workload.h"
#include "header.h"

#include "spdk/env.h"
#include "spdk/nvme.h"
#include "spdk/stdinc.h"

namespace io_bench {

enum io_type_t {
    IO_SEQ,
    IO_RANDOM
};

enum workload_type_t {
    WORKLOAD_DBBENCH,
    WORKLOAD_YCSB,
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

    // 负载发生器
    workload_type_t workload_type;

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

        num_write_thread = 1;

        time_based = false;

        time = 0;

        block_size = 4096;

        io_size = 8UL * 1024 * 1024 * 1024;
    }
};

// ------------------------------------------------------

class IOHandle {
public:
    virtual ~IOHandle(){};

    virtual void Run() = 0;

    virtual void Print() = 0;
};

// ------------------------------------------------------

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
};

// ------------------------------------------------------

class SPDKDevice {
public:
    int used;
    std::string name;
    std::string transport_string;
    struct spdk_nvme_transport_id trid;

public:
    struct spdk_nvme_ctrlr* ctrlr;
    const struct spdk_nvme_ctrlr_opts* opts;
    const struct spdk_nvme_ctrlr_data* cdata;

public:
    uint32_t num_ns;
    uint64_t ns_capacity;
    struct spdk_nvme_ns* ns;
};

class SpdkIOHandle : public IOHandle {
public:
    SpdkIOHandle(IO_Options* options);

    ~SpdkIOHandle();

public:
    void Run();

    void Print();

private:
    IO_Options* options_;

    SPDKDevice device_;

    char result_save_path_[128];
};
};

#endif