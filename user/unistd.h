#ifndef UNISTD_H
#define UNISTD_H

#define SEEK_SET    0
#define SEEK_CUR    1
#define SEEK_END    2

#define SEEK_MAX    3


int close(int fd);

int read(int fd, void * buf, unsigned long count);

int write(int fd, void * buf, unsigned long count);

int lseek(int fd, long offset, int whence);

int fork();

int vfork();

int execve(char *path);



#endif