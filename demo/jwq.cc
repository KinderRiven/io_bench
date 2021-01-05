#define __USE_GNU 1
#include <aio.h>
#include <errno.h>
#include <fcntl.h>
#include <header.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/uio.h>

#define BUFFER_SIZE 4096
#define PARALLEL_SIZE 16

int main(int argc, char* argv[])
{
    int i;
    int fd;
    int io_size = atoi(argv[1]);
    printf("%d\n", io_size);
    char buffer[BUFFER_SIZE];
    // char *buffer;
    // posix_memalign((void *)&buffer, BUFFER_SIZE, BUFFER_SIZE);
    memset(buffer, 'c', BUFFER_SIZE);
    fd = open("test_write", O_RDWR | O_DIRECT, 0666);
    for (i = 0; i < io_size * 256; i++) {
        write(fd, buffer, BUFFER_SIZE);
        // sync();
    }
    close(fd);
    return 0;
}