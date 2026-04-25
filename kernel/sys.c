#include "printk.h"
#include "errno.h"
#include "memory.h"
#include "lib.h"
#include "vfs.h"
#include "fcntl.h"

unsigned long no_system_call(){
    ColorPrintfk(RED, BLACK, "here is no_system_call\n");
    return -ENOSYS;
}

unsigned long sys_putstring(char *string){
    ColorPrintfk(GREEN, BLACK, "here is sys_putstring\n");
    ColorPrintfk(BLUE, BLACK, "%s\n", string);
    return 0;
}

unsigned long sys_open(char * filename, int flag){
    char * path = NULL;
    long errno  = 0;
    long fd     = 0;
    int i       = 0;
    file ** f   = NULL;

    ColorPrintfk(GREEN, BLACK, "here is sys_open\n");
    path = (char *)kmalloc(PAGE_4K_SIZE, 0);
    if(path == NULL) return -ENOMEM;
    memset(path, 0, PAGE_4K_SIZE);
    long pathlen = strnlen_user(filename, PAGE_4K_SIZE);
    if(pathlen <= 0){
        kfree(path);
        return -EFAULT;
    }else if (pathlen >= PAGE_4K_SIZE){
        kfree(path);
        return -ENAMETOOLONG;
    }
    strncpy_from_user(filename, path, pathlen);

    dir_entry * dentry = path_walk(path, 0);
    kfree(path);

    if(dentry == NULL) {
        ColorPrintfk(BLUE, BLACK, "Can`t Find file %s", path);
        return -ENOENT;
    }

    ColorPrintfk(BLUE, BLACK, "entry name : %s, entry size : %d\n", dentry->name, dentry->dir_node->file_size);
    if(dentry->dir_node->attribute == FS_ATTR_DIR) return -EISDIR;

    file * filp = (file *) kmalloc(sizeof(file), 0);
    memset(filp, 0, sizeof(file));
    filp->dentry = dentry;
    filp->f_ops = dentry->dir_node->f_ops;
    filp->mode = flag;

    if(filp->f_ops && filp->f_ops->open)
        errno = filp->f_ops->open(dentry->dir_node, filp);
    
    if(errno != 1){
        kfree(filp);
        return -EFAULT;
    }

    if(filp->mode & O_TRUNC){
        filp->dentry->dir_node->file_size = 0;
    }

    if(filp->mode & O_APPEND){
        filp->position = filp->dentry->dir_node->file_size;
    }

    f = CURRENT->handle_array;

    for(i = 0 ; i < MAX_HANDLE_PER_TASK; i++){
        if(*(f + i) == NULL){
            fd = i;
            break;
        }
    }
    if(i == MAX_HANDLE_PER_TASK){
        kfree(filp);

        // TO DO FREE Dentry and inode
        return -EMFILE;
    }

    f[fd] = filp;

    ColorPrintfk(BLUE, BLACK, "fd: %d\n", fd);

    return fd;
}

unsigned long sys_close(int fd){

    if(fd < 0 || fd >= MAX_HANDLE_PER_TASK){
        return -EBADF;
    }
    file * filp = CURRENT->handle_array[fd];

    if(filp->f_ops && filp->f_ops->close) 
        filp->f_ops->close(filp->dentry->dir_node, filp);
    
    kfree(filp);

    CURRENT->handle_array[fd] = NULL;
    return 0;
}


unsigned long sys_read(int fd, void * buf, unsigned long count){

    long ret;
    if(fd < 0 || fd >= MAX_HANDLE_PER_TASK){
        return -EBADF;
    }

    if(count < 0 ){
        return -EINVAL;
    }

    ColorPrintfk(BLUE, BLACK, "count : %d\n", count);
    file * filp = CURRENT->handle_array[fd];

    if(filp->f_ops && filp->f_ops->close) 
        ret = filp->f_ops->read(filp, buf, count, &filp->position);
    return ret;
}