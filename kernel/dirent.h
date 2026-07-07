#ifndef DIRENT_H
#define DIRENT_H

typedef struct DIR{
    int fd;
    int buf_pos;
    int buf_end;
    char buf[256];
}DIR;

typedef struct dirent{
    long d_offset;
    long d_namelen;
    long d_type;
    char d_name[];
}dirent;

DIR * opendir(const char *dirname);
int closedir(DIR *dirp);
dirent * readdir(DIR * dirp);


#endif