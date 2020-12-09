#include "run_workload.h"
#include "timer.h"

using namespace io_bench;

static volatile int g_stop = 0;

struct io_thread_t {
public:
    Workload* workload;

public:
    // DDDD
    int thread_id;

    // 0是读，1是写
    int rw;

    // IO类型：随机或顺序
    io_type_t io_type;

    //io_qpair
    int io_depth;

    struct spdk_nvme_qpair* io_qpair;

    uint64_t io_space_size;

    // 一共要进行的IO量
    size_t io_total_size;

    // IO的粒度
    size_t io_block_size;

    // 是否基于时间运行
    bool time_based;

    // 运行多长时间停止
    int time;

public:
    // 记录每个请求的延迟
    std::vector<uint64_t> vec_latency;

    // 所有的请求延迟和
    uint64_t total_time;

    // 平均时间
    double avg_time;

    // IOPS
    double iops;
};

static bool probe_cb(void* cb_ctx, const struct spdk_nvme_transport_id* trid, struct spdk_nvme_ctrlr_opts* opts)
{
}

static void attach_cb(void* cb_ctx, const struct spdk_nvme_transport_id* trid, struct spdk_nvme_ctrlr* ctrlr, const struct spdk_nvme_ctrlr_opts* opts)
{
    SPDKDevice* device = ((SPDKDevice*)cb_ctx);
    device->ctrlr = ctrlr;
    device->opts = opts;
    device->cdata = spdk_nvme_ctrlr_get_data(ctrlr);
    device->num_ns = spdk_nvme_ctrlr_get_num_ns(ctrlr);
    // We assume that one device only cconfigure one namespace
    assert(device->num_ns == 1);
    device->ns = spdk_nvme_ctrlr_get_ns(ctrlr, 1);
    device->ns_capacity = spdk_nvme_ns_get_size(device->ns);
}

static void remove_cb(void* cb_ctx, struct spdk_nvme_ctrlr* ctrlr)
{
}

static void run_io_thread(io_thread_t* io_thread)
{
    bool _time_based = io_thread->time_based;
    uint64_t _run_time = (uint64_t)io_thread->time * 1000000000UL;
    Timer _timer;
    Workload* _workload = io_thread->workload;

    size_t _io_space_size = io_thread->io_space_size;
    size_t _io_total_size = io_thread->io_total_size;
    size_t _io_block_size = io_thread->io_block_size;

    int _io_depth = io_thread->io_depth;
    uint64_t _space_count = _io_space_size / _io_block_size;
    uint64_t _do_count = (_time_based == true) ? 0 : _io_total_size / _io_block_size;
    _do_count /= _io_depth;

    // 为了测试效果最好，需要进行4KB的对齐
    uint64_t _io_start = 0;
    uint64_t _io_end = _io_start + io_thread->io_space_size;
    uint64_t _pos = _io_start;

    // DIRECT_IO需要512B的对齐，为了测试效果最好，进行4KB的内存申请
    void* _buff;
    posix_memalign(&_buff, 4096, _io_block_size);
    memset(_buff, 0xff, _io_block_size);
    assert(_io_start % 4096 == 0);

    printf("[thread:%02d][fd:%d][time:%dseconds][start:%lluMB][end:%lluMB][SC:%llu][BS:%zuB][SIZE:%zuMB][COUNT:%llu]\n",
        io_thread->thread_id, _fd, io_thread->time, _io_start / (1024 * 1024), _io_end / (1024 * 1024), _space_count,
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
        // TODO IO
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
        // TODO IO
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
        // TODO IO
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
        // TODO IO
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

SpdkIOHandle::SpdkIOHandle(IO_Options* options)
    : options_(options)
{
    // 建立结果输出文件夹
    time_t _t = time(NULL);
    struct tm* _lt = localtime(&_t);
    sprintf(result_save_path_, "%04d%02d%02d_%02d%02d%02d", _lt->tm_year, _lt->tm_mon, _lt->tm_mday, _lt->tm_hour, _lt->tm_min, _lt->tm_sec);
    mkdir(result_save_path_, 0666);
    // SPDK init device
    int _res;
    spdk_env_opts_init(&env_opts_);
    _res = spdk_env_init(&env_opts_);
    _res = spdk_nvme_transport_id_parse(&device_.trid, device_.transport_string.c_str());
    _res = spdk_nvme_probe(&device_.trid, &device_, probe_cb, attach_cb, remove_cb);
}

SpdkIOHandle::~SpdkIOHandle()
{
}

void SpdkIOHandle::Run()
{
    io_thread_t io_threads_[64];
    std::thread threads_[64];

    int _thread_id = 0;
    size_t _per_thread_io_size = options_->io_size / (options_->num_write_thread + options_->num_read_thread);
    size_t _per_thread_io_space_size = options_->space_size / (options_->num_write_thread + options_->num_read_thread);

    for (int i = 0; i < options_->num_write_thread; i++, _thread_id++) {
        // 传参
        io_threads_[_thread_id].thread_id = _thread_id;
        io_threads_[_thread_id].io_qpair = spdk_nvme_ctrlr_alloc_io_qpair(device_.ctrlr, nullptr, 0);
        io_threads_[_thread_id].io_depth = 8;
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
        io_threads_[_thread_id].io_qpair = spdk_nvme_ctrlr_alloc_io_qpair(device_->ctrlr, nullptr, 0);
        io_threads_[_thread_id].io_depth = 8;
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
    }
    for (int i = 0; i < options_->num_read_thread; i++, _thread_id++) {
        threads_[_thread_id].join();
    }
}

void SpdkIOHandle::Print()
{
}
