#include "header.h"
#include "timer.h"

#define DO_RW (1)
#define DO_SW (2)
#define DO_RR (3)
#define DO_SR (4)

struct thread_options {
    int type;
    int thread_id;
    size_t block_size;
    size_t total_size;
    double iops;
    char path[128];
};

static int io_depth = 16;

// async (libaio)
static int io_destroy(aio_context_t ctx)
{
    return syscall(__NR_io_destroy, ctx);
}

static int io_setup(unsigned nr, aio_context_t* ctxp)
{
    return syscall(__NR_io_setup, nr, ctxp);
}

static int io_submit(aio_context_t ctx, long nr, struct iocb** iocbpp)
{
    return syscall(__NR_io_submit, ctx, nr, iocbpp);
}

static int io_getevents(aio_context_t ctx, long min_nr, long max_nr,
    struct io_event* events, struct timespec* timeout)
{
    return syscall(__NR_io_getevents, ctx, min_nr, max_nr, events, timeout);
}

void do_aio_write(int fd, size_t total_size, size_t block_size)
{
    int ret;
    void* vbuff;
    size_t queue_size = io_depth;
    size_t current_count = 0;
    posix_memalign(&vbuff, block_size, block_size * queue_size);
    char* buff = (char*)vbuff;
    memset(buff, 0xff, block_size * queue_size);
    size_t count = total_size / (block_size * queue_size);

    aio_context_t ioctx;
    struct io_event events[128];
    struct iocb* iocbs[128];
    struct iocb iocb[128];

    ioctx = 0;
    io_setup(128, &ioctx);
    for (int i = 0; i < count; i++) {
        for (int j = 0; j < queue_size; j++) {
            iocb[j].aio_fildes = fd;
            iocb[j].aio_nbytes = block_size;
            iocb[j].aio_offset = block_size * current_count;
            iocb[j].aio_lio_opcode = IOCB_CMD_PWRITE;
            iocb[j].aio_buf = (uint64_t)&buff[j * block_size];
            iocbs[j] = &iocb[j];
            current_count++;
        }
        ret = io_submit(ioctx, queue_size, iocbs);
        assert(ret == queue_size);
        ret = io_getevents(ioctx, ret, ret, events, nullptr);
        assert(ret == queue_size);
    }
    io_destroy(ioctx);
    free(vbuff);
}

void do_aio_read(int fd, size_t total_size, size_t block_size)
{
    int ret;
    void* vbuff;
    size_t queue_size = io_depth;
    size_t current_count = 0;
    posix_memalign(&vbuff, block_size, block_size * queue_size);
    char* buff = (char*)vbuff;
    memset(buff, 0xff, block_size * queue_size);
    size_t count = total_size / (block_size * queue_size);
    aio_context_t ioctx;
    struct iocb iocb[128];
    struct io_event events[128];
    struct iocb* iocbs[128];

    ioctx = 0;
    io_setup(128, &ioctx);
    for (int i = 0; i < count; i++) {
        for (int j = 0; j < queue_size; j++) {
            iocb[j].aio_fildes = fd;
            iocb[j].aio_nbytes = block_size;
            iocb[j].aio_offset = block_size * current_count;
            iocb[j].aio_lio_opcode = IOCB_CMD_PREAD;
            iocb[j].aio_buf = (uint64_t)&buff[j * block_size];
            iocbs[j] = &iocb[j];
            current_count++;
        }
        ret = io_submit(ioctx, queue_size, iocbs);
        assert(ret == queue_size);
        ret = io_getevents(ioctx, ret, ret, events, nullptr);
        assert(ret == queue_size);
    }
    io_destroy(ioctx);
    free(vbuff);
}

// #define USE_FALLOCATE
int main(int argc, char** argv)
{
    Timer _timer;
    int _scan;
    char* _name = argv[1];
    size_t _size = atol(argv[2]) * 1024 * 1024;
    size_t _block_size = atol(argv[3]);

    printf("[%s][size:%zu][bs:%zu]\n", _name, _size, _block_size);

    int _fd = open(_name, O_RDWR | O_DIRECT, 0666);
    assert(_fd > 0);

#if 1
    _timer.Start();
    do_aio_read(_fd, _size, _block_size);
    _timer.Stop();
    printf("read time:%.2fseconds\n", 1.0 * _timer.Get() / 1000000000);
#endif

#if 0
    _timer.Start();
    do_aio_write(_fd, _size, _block_size);
    _timer.Stop();
    printf("write time:%.2fseconds\n", 1.0 * _timer.Get() / 1000000000);
#endif
    close(_fd);
    return 0;
}