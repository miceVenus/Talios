#include "dirent.h"
#include "stdlib.h"
#include "fcntl.h"
#include "unistd.h"

DIR * opendir(const char * path){
    int fd = 0;
    DIR * dir = NULL;
    fd = open(path, O_DIRECTORY);

    if(fd >= 0)
        dir = (DIR *)malloc(sizeof(DIR));
    else return NULL;

    memset(dir, 0, sizeof(DIR));

    dir->buf_pos    = 0;
    dir->buf_end    = 256;
    dir->fd         = fd;

    return dir;
}

int closedir(DIR *dir){
    close(dir->fd);
    free((void *)dir);
    return 0;
}

dirent * readdir(DIR * dir){
    if(dir->fd < 0) return NULL;

    memset(dir->buf, 0, 256);

    int len = getdents(dir->fd, (dirent *)dir->buf, 256);
    if(len > 0) return (dirent *)dir->buf;
    else return NULL;
    
}