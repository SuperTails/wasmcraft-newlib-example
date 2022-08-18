#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <stdbool.h>
#include <reent.h>
#include <stdlib.h>

#include "mc_mman.h"
#include "mcinterface.h"

/* ======================================== */
/* ============== malloc ================== */
/* ======================================== */

// You can adjust this to change the amount of memory available to malloc.
// However, setting this to much higher values (like 128 * 65536)
// will cause severe lag and possibly make the world unloadable.
#define MEMORY_SIZE (4 * 65536)

void *_calloc_r(struct _reent* r, size_t num, size_t size) {
  return calloc(num, size);
}

void *_malloc_r(struct _reent* r, size_t size) {
  return malloc(size);
}

void _free_r(struct _reent* r, void *ptr) {
  free(ptr);
}

void *_realloc_r(struct _reent* r, void *ptr, size_t new_size) {
  return realloc(ptr, new_size);
}

uint8_t memory[MEMORY_SIZE] = {};

uint8_t *next = memory;

void *sbrk(intptr_t increment) {
  uint8_t *result = next;

  intptr_t current_size = (intptr_t)next - (intptr_t)memory;

  if (current_size + increment >= MEMORY_SIZE) {
    errno = ENOMEM;
    return NULL;
  }

  next += increment;

  return result;
}

/* ================================ */
/* ========= Filesystem =========== */
/* ================================ */

// Minecraft doesn't have a real filesystem [Citation Needed].
// In order to emulate programs that want to open specific files with minimal modification,
// the simplest solution is to store each file in the binary itself as a C array,
// and hardcode the path each file would have.

// This represents one of our fake files.
typedef struct {
    const char *name;
    const unsigned char *data;
    int size;
} stub_file;

// There are many programs available to convert a file into a C array, so take your pick.
const unsigned char file_foo_data[] = { 'T','h','i','s',' ','i','s',' ','f','o','o','\n' };
const int file_foo_size = sizeof(file_foo_data) / sizeof(file_foo_data[0]);

const unsigned char file_bar_data[] = { 'B', 'A', 'R' };
const int file_bar_size = sizeof(file_bar_data) / sizeof(file_bar_data[0]);

#define NUM_STUB_FILES 2

stub_file stub_files[NUM_STUB_FILES] = {
    { "some/path/foo.txt", file_foo_data, file_foo_size },
    { "another/file/bar.txt", file_bar_data, file_bar_size }
};

// Here's the info for a single open file descriptor.
typedef struct {
    stub_file *file;
    off_t offset;
    bool valid;
} fd_info;


#define MAX_FDS 16

// A file descriptor returned from e.g. `open` is just an index into this array.
fd_info fd_table[MAX_FDS] = {};

#define STDOUT_FD 0
#define STDERR_FD 1
#define STDIN_FD 2

int _open(const char *pathname, int flags, mode_t mode) {
    int file_idx = -1;
    for (int i = 0; i < NUM_STUB_FILES; i++) {
        if (strcmp(pathname, stub_files[i].name) == 0) {
            file_idx = i;
            break;
        }
    }

    if (file_idx == -1) {
        errno = ENOENT;
        return -1;
    }

    int fd = -1;
    for (int i = 3; i < MAX_FDS; ++i) {
        if (!fd_table[i].valid) {
            fd = i;
            break;
        }
    }

    if (fd == -1) {
        errno = ENFILE;
        return -1;
    }

    fd_table[fd].file = &stub_files[file_idx];
    fd_table[fd].valid = true;
    fd_table[fd].offset = 0;
    return fd;
}

int open(const char *pathname, int flags, mode_t mode) {
    return _open(pathname, flags, mode);
}

int _close(int fd) {
    if (0 <= fd && fd < MAX_FDS) {
        if (fd_table[fd].valid) {
            fd_table[fd].valid = false;
            return 0;
        }
    }

    errno = EBADF;
    return -1;
}

int close(int fd) {
    return _close(fd);
}

int _link(const char *oldpath, const char *newpath) {
  const char msg[] = "ignoring link\n";
  _write(STDOUT_FD, msg, sizeof(msg) - 1);

  errno = ENOSYS;
  return -1;
}

int _unlink(const char *pathname) {
  const char msg[] = "ignoring unlink\n";
  _write(STDOUT_FD, msg, sizeof(msg) - 1);

  errno = ENOSYS;
  return -1;
}

/* ===================================== */
/* ======== Read/Write/Seek ============ */
/* ===================================== */


#define MAX_READ_BATCH_SIZE (1000) 
int _read(int fd, void *buf, size_t count) {
  uint8_t *buf_bytes = (uint8_t *)buf;

  if (fd < 0 || fd >= MAX_FDS) {
    errno = EBADF;
    return -1;
  }

  fd_info *info = &fd_table[fd];
  if (!info->valid) {
    errno = EBADF;
    return -1;
  }

  int file_size = info->file->size;
  uint8_t *file_data = (uint8_t*)info->file->data;

  ssize_t num_read = 0;
  while (count > 0) {
    if (info->offset == file_size) {
      break;
    }

    ssize_t file_bytes_remaining = file_size - info->offset;

    ssize_t max_read_ahead = file_bytes_remaining;
    if (max_read_ahead > count) {
      max_read_ahead = count;
    }
    if (max_read_ahead > MAX_READ_BATCH_SIZE) {
      max_read_ahead = MAX_READ_BATCH_SIZE;
    }

    memcpy(buf_bytes, file_data + info->offset, max_read_ahead);

    buf_bytes += max_read_ahead;
    info->offset += max_read_ahead;
    num_read += max_read_ahead;
    count -= max_read_ahead;

    if (max_read_ahead == MAX_READ_BATCH_SIZE) {
      mc_sleep();
    }
  }

  return num_read;
}

int read(int fd, void *buf, size_t count) {
    return _read(fd, buf, count);
}

int _write(int fd, const void *buf, size_t count) {
  if (fd == STDOUT_FD || fd == STDERR_FD) {
    uint8_t *buf_bytes = (uint8_t*)buf;
    for (size_t i = 0; i < count; ++i) {
      mc_putc(buf_bytes[i]);
    }
    return count;
  } else {
    errno = ENOSYS;
    return -1;
  }
}

int write(int fd, const void *buf, size_t count) {
    return _write(fd, buf, count);
}

void *mmap(void *addr, size_t length, int prot, int flags, int fd, off_t offset) {
  if (prot != (PROT_READ | PROT_WRITE)) {
    errno = ENOSYS;
    return (void*)-1;
  }

  if (flags != MAP_PRIVATE) {
    errno = ENOSYS;
    return (void*)-1;
  }

  if (addr != NULL) {
    errno = ENOSYS;
    return (void*)-1;
  }

  if (fd < 0 || fd >= MAX_FDS) {
    errno = EBADF;
    return (void*)-1;
  }

  fd_info *info = &fd_table[fd];
  if (!info->valid) {
    errno = EBADF;
    return (void*)-1;
  }

  // Discards qualifiers but oh well.
  // hope you don't write to it!
  return (void*)info->file->data;
}

off_t _lseek(int fd, off_t offset, int whence) {
  if (fd < 0 || fd >= MAX_FDS) {
    errno = EBADF;
    return (off_t)-1;
  }

  fd_info *info = &fd_table[fd];
  if (!info->valid) {
    errno = EBADF;
    return (off_t)-1;
  }

  int file_size = info->file->size;

  off_t new_offset;

  switch (whence) {
  case SEEK_SET:
    new_offset = offset;
    break;
  case SEEK_CUR:
    new_offset = info->offset + offset;
    break;
  case SEEK_END:
    new_offset = file_size + offset;
    break;
  default:
    errno = EINVAL;
    return (off_t)-1;
  }

  if (new_offset < 0 || new_offset > file_size) {
    errno = EINVAL;
    return (off_t)-1;
  }

  info->offset = new_offset;
  return info->offset;
}

/* ===================================== */
/* =========== Miscellaneous =========== */
/* ===================================== */

int munmap(void *addr, size_t length) {
  return 0;
}

void _exit(int code) {
    const char msg[] = "exit\n";
    _write(STDOUT_FD, msg, sizeof(msg) - 1);

    // We don't have arbitrary control flow, so this is really the only thing you can do.
    while (1) {}
}

// wasmcraft doesn't support doubles anyway; this is just to make the linker happy.
long double __extenddftf2(double a) { return 0.0; }
double __trunctfdf2(long double a) { return 0.0; }

int mkdir(const char *pathname, mode_t mode) {
  const char msg[] = "ignoring mkdir\n";
  _write(STDOUT_FD, msg, sizeof(msg) - 1);

  errno = ENOSYS;
  return -1;
}

int _fstat(int fd, struct stat *statbuf) {
  if (fd < 0 || fd >= MAX_FDS) {
    errno = EBADF;
    return -1;
  }
  
  fd_info *info = &fd_table[fd];
  if (!info->valid) {
    errno = EBADF;
    return -1;
  }

  int size = info->file->size;

  //statbuf->st_mode = ???
  statbuf->st_nlink = 1;
  statbuf->st_size = size;
  statbuf->st_blksize = 512;
  statbuf->st_blocks = size / (512 * 1024);

  return 0;
}

int raise(int sig) {
  const char msg[] = "signal raised\n";
  _write(STDOUT_FD, msg, sizeof(msg) - 1);

  // TODO:
  while (1) {}
}