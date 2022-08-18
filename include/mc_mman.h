#include <stdint.h>
#include <stddef.h>
#include <unistd.h>

#define PROT_READ   (1 << 0)
#define PROT_WRITE  (1 << 1)

#define MAP_PRIVATE (1 << 2)

void *mmap(void *addr, size_t length, int prot, int flags, int fd, off_t offset);

int munmap(void *addr, size_t length);