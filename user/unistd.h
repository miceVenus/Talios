#ifndef UNISTD_H
#define UNISTD_H

#define SEEK_SET    0
#define SEEK_CUR    1
#define SEEK_END    2

#define SEEK_MAX    3


int close(int fd);

int read(int fd, void * buf, unsigned long count);

int write(int fd, void * buf, unsigned long count);

long lseek(int fd, long offset, int whence);

void _exit(int status) __attribute__((noreturn));

int fork();

int vfork();

int execve(char *path);

int chdir(const char *path);

int fchdir(int fd);



#endif