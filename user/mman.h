#ifndef TALIOS_USER_MMAN_H
#define TALIOS_USER_MMAN_H
#include "../include/mman.h"

void *mmap(void *addr, unsigned long length, int prot, int flags, int fd, long offset);
int munmap(void *addr, unsigned long length);
int msync(void *addr, unsigned long length, int flags);

#endif
