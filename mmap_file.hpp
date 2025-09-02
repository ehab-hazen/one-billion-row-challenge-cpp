#pragma once

#include <cstddef>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

class MMapFile {
  public:
    MMapFile(const char *path) {
        fd = open(path, O_RDONLY);
        if (fd == -1) {
            perror("open");
            exit(1);
        }

        length = lseek(fd, 0, SEEK_END);
        if (length == (off_t)-1) {
            perror("lseek");
            exit(1);
        }

        ptr = mmap(NULL, length, PROT_READ, MAP_PRIVATE, fd, 0);
        if (ptr == MAP_FAILED) {
            perror("mmap");
            exit(1);
        }
    }

    ~MMapFile() {
        if (munmap(ptr, length) == -1) {
            perror("munmap");
            exit(1);
        }
        if (close(fd) == -1) {
            perror("close");
            exit(1);
        }
    }

    void *data() const { return ptr; }

    const char *begin() const { return static_cast<const char *>(ptr); }
    const char *end() const { return static_cast<const char *>(ptr) + length; }

    size_t size() const { return length; }

  private:
    void *ptr = nullptr;
    size_t length = 0;
    int fd = -1;
};
