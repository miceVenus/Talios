#include "printk.h"
#include "errno.h"
#include "memory.h"
#include "lib.h"
#include "vfs.h"
#include "unistd.h"
#include "fcntl.h"
#include "stdio.h"
#include "sched.h"
#include "task.h"
#include "fat32.h"
#include "dirent.h"
#include "sys.h"


extern file_operations keyboard_operation;

unsigned long no_system_call(){
    ColorPrintfk(RED, BLACK, "here is no_system_call\n");
    return -ENOSYS;
}

unsigned long sys_putstring(char *string){
    // ColorPrintfk(GREEN, BLACK, "here is sys_putstring\n");
    ColorPrintfk(BLUE, BLACK, "%s", string);
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
    
    if(!(flag & O_DIRECTORY) && (dentry->dir_node->attribute == FS_ATTR_DIR)) return -EISDIR;

    if((flag & O_DIRECTORY) && (dentry->dir_node->attribute != FS_ATTR_DIR)) return -ENOTDIR;


    file * filp = (file *) kmalloc(sizeof(file), 0);
    memset(filp, 0, sizeof(file));
    filp->dentry = dentry;
    filp->mode = flag;

    if(dentry->dir_node->attribute & FS_ATTR_DEVICE){
        filp->f_ops = &keyboard_operation;
    }else{
        filp->f_ops = dentry->dir_node->f_ops;
    }

    
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

    // if(filp->mode & O_DIRECTORY){
    //     filp->f_ops = filp->dentry->dir_ops;
    // }

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

    unsigned long res;
    if(fd < 0 || fd >= MAX_HANDLE_PER_TASK){
        return -EBADF;
    }
    file * filp = CURRENT->handle_array[fd];

    if(filp->f_ops && filp->f_ops->close) 
        res = filp->f_ops->close(filp->dentry->dir_node, filp);

    kfree(filp);

    CURRENT->handle_array[fd] = NULL;
    return res;
}

unsigned long sys_ioctl(int fd, unsigned long request, void * args){
    unsigned long res;
    if(fd < 0 || fd >= MAX_HANDLE_PER_TASK){
        return -EBADF;
    }

    file * filp = CURRENT->handle_array[fd];
    if(filp->f_ops && filp->f_ops->ioctl){
        res = filp->f_ops->ioctl(filp->dentry->dir_node, filp, request, (unsigned long)args);
    }
    return res;
}


unsigned long sys_read(int fd, void * buf, unsigned long count){

    // ColorPrintfk(BLUE, BLACK, "here is read");
    long ret;
    if(fd < 0 || fd >= MAX_HANDLE_PER_TASK){
        return -EBADF;
    }

    if(count < 0 ){
        return -EINVAL;
    }

    file * filp = CURRENT->handle_array[fd];

    if(filp->f_ops && filp->f_ops->close) 
        ret = filp->f_ops->read(filp, buf, count, &filp->position);
    return ret;
}

unsigned long sys_write(int fd, void * buf, unsigned long count){
    long ret;
    if(fd < 0 || fd >= MAX_HANDLE_PER_TASK){
        return -EBADF;
    }

    if(count < 0){
        return -EINVAL;
    }

    ColorPrintfk(BLUE, BLACK, "count : %d\n", count);
    file * filp = CURRENT->handle_array[fd];

    if(filp->f_ops && filp->f_ops->close){
        ret = filp->f_ops->write(filp, buf, count, &filp->position);
    }

    return ret;
}

unsigned long sys_lseek(int fd, long offset, int whence){
    long ret;

    if(fd < 0 || fd >= MAX_HANDLE_PER_TASK){
        return -EBADF;
    }

    if(whence < 0 || whence >= SEEK_MAX){
        return -EINVAL;
    }

    file * filp = CURRENT->handle_array[fd];

    if(filp->f_ops && filp->f_ops->close){
        ret = filp->f_ops->lseek(filp, offset, whence);
    }

    return ret;
}

unsigned long sys_fork(){
    long ret;

    struct PtRegs * regs = (struct PtRegs *)CURRENT->thread->rsp0 - 1;

    ColorPrintfk(GREEN, BLACK, "here is fork");

    return do_fork(regs, 0, regs->rsp, 0);
}

unsigned long sys_vfork(){
    long ret;

    struct PtRegs * regs = (struct PtRegs *)CURRENT->thread->rsp0 - 1;

    ColorPrintfk(GREEN, BLACK, "here is fork");

    return do_fork(regs, CLONE_FS | CLONE_SIGNAL | CLONE_VM, regs->rsp, 0);
}

unsigned long sys_execve(char *path){
    long ret;

    struct PtRegs * regs = (struct PtRegs *)CURRENT->thread->rsp0 - 1;

    ColorPrintfk(GREEN, BLACK, "here is execve");

    return do_execve(regs, path);
}

unsigned long sys_brk(unsigned long brk){

    TaskStruct *cur = CURRENT;

    unsigned long new_brk = PAGE_4K_ALIGN_UP(brk);
    if(new_brk == 0)
        return cur->lmm->StartBrk;
    
    if(new_brk < cur->lmm->EndBrk)
        return 0;                       // release brk space

    new_brk = do_brk(cur->lmm->EndBrk, new_brk - cur->lmm->EndBrk);

    cur->lmm->EndBrk = new_brk;

    return new_brk;
}

unsigned long sys_reboot(unsigned long cmd, void *arg){

    switch (cmd){
        case SYSTEM_REBOOT:
            OUT8b(0x64, 0xfe);
            break;
        case SYSTEM_POWEROFF:
            ColorPrintfk(BLUE, BLACK, "what are you expecting");
            break;
        default:
            ColorPrintfk(RED, BLACK, "no this cmd %d", cmd);
            break;
    }

    return 0;

}

unsigned long sys_chdir(const char *filename){

    char *path = NULL;
    long pathlen = 0;
    dir_entry * dentry = NULL;

    path = (char *)kmalloc(PAGE_4K_SIZE, 0);
    if(!path) return -ENOMEM;

    memset(path, 0, PAGE_4K_SIZE);

    pathlen = strnlen_user(filename, PAGE_4K_SIZE);

    if(pathlen <= 0){
        kfree(path);
        return -EFAULT;
    }

    if(pathlen == PAGE_4K_SIZE){
        kfree(path);
        return -ENAMETOOLONG;
    }

    strncpy_from_user(filename, path, pathlen);

    dentry = path_walk(path, 0);
    kfree(path);

    if(!dentry) return -ENOENT;

    if(dentry->dir_node->attribute != FS_ATTR_DIR)
        return -ENOTDIR;

    return 0;

}

unsigned long sys_fchdir(unsigned int fd){
    
    return 0;

}

int fill_dentry(void * buf, char * name, long namelen, long type, long offset){
    dirent * entry = (dirent *)buf;
    if((unsigned long)buf < TASK_SIZE && !verify_area(buf, sizeof(dirent) + namelen))
        return -EFAULT;

    memcopy(name, entry->d_name, namelen);
    entry->d_namelen = namelen;
    entry->d_offset = offset;
    entry->d_type = type;

    return sizeof(dirent) + namelen;
}


unsigned long sys_getdents(int fd, void * dirent, long count){
    long ret;
    if(fd < 0 || fd >= MAX_HANDLE_PER_TASK){
        return -EBADF;
    }

    if(count < 0 ){
        return -EINVAL;
    }

    file * filp = CURRENT->handle_array[fd];

    if(filp->f_ops && filp->f_ops->close) 
        ret = filp->f_ops->readdir(filp, dirent, fill_dentry);
    return ret;
}
