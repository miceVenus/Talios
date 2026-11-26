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
#include "mmap.h"


extern file_operations keyboard_operation;

unsigned long no_system_call(){
    ColorPrintfk(RED, BLACK, "here is no_system_call\n");
    return -ENOSYS;
}

unsigned long sys_putstring(char *string){
    long len = strnlen_user(string, PAGE_4K_SIZE - 1);
    if(len < 0) return -EFAULT;
    char *copy = kmalloc(len + 1, 0);
    if(!copy) return -ENOMEM;
    if(strncpy_from_user(string, copy, len)){ kfree(copy); return -EFAULT; }
    ColorPrintfk(BLUE, BLACK, "%s", copy);
    kfree(copy);
    return 0;
}

unsigned long sys_open(char * filename, int flag){
    char * path = NULL;
    long errno  = 0;
    long fd     = 0;
    int i       = 0;


    if((flag & O_ACCMODE) == O_ACCMODE) return -EINVAL;
    path = (char *)kmalloc(PAGE_4K_SIZE, 0);
    if(path == NULL) return -ENOMEM;
    memset(path, 0, PAGE_4K_SIZE);
    long pathlen = strnlen_user(filename, PAGE_4K_SIZE);
    if(pathlen <= 0){
        kfree(path);
        return -EFAULT;
    }else if (pathlen >= (long)PAGE_4K_SIZE){
        kfree(path);
        return -ENAMETOOLONG;
    }
    if(strncpy_from_user(filename, path, pathlen)){ kfree(path); return -EFAULT; }

    dir_entry * dentry = path_walk(path, 0);
    kfree(path);

    if(dentry == NULL) {

        return -ENOENT;
    }

    ColorPrintfk(BLUE, BLACK, "entry name : %s, entry size : %d\n", dentry->name, dentry->dir_node->file_size);
    
    if(!(flag & O_DIRECTORY) && (dentry->dir_node->attribute == FS_ATTR_DIR)) return -EISDIR;

    if((flag & O_DIRECTORY) && (dentry->dir_node->attribute != FS_ATTR_DIR)) return -ENOTDIR;


    file * filp = (file *) kmalloc(sizeof(file), 0);
    if(!filp) return -ENOMEM;
    memset(filp, 0, sizeof(file));
    filp->refs = 1;
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

    for(i = 0; i < MAX_HANDLE_PER_TASK && CURRENT->handle_array[i]; i++) {}
    if(i == MAX_HANDLE_PER_TASK){ file_put(filp); return -EMFILE; }
    fd = i;
    if(filp->mode & O_TRUNC){
        if((flag & O_ACCMODE) == O_RONLY || dentry->dir_node->attribute != FS_ATTR_FILE){
            file_put(filp);
            return -EACCES;
        }
        errno = vm_file_truncate(filp);
        if(errno){ file_put(filp); return errno; }
    }

    if(filp->mode & O_APPEND){
        filp->position = filp->dentry->dir_node->file_size;
    }

    // if(filp->mode & O_DIRECTORY){
    //     filp->f_ops = filp->dentry->dir_ops;
    // }

    CURRENT->handle_array[fd] = filp;

    ColorPrintfk(BLUE, BLACK, "fd: %d\n", fd);

    return fd;
}

static file *get_fd(int fd){
    return fd >= 0 && fd < MAX_HANDLE_PER_TASK ? CURRENT->handle_array[fd] : NULL;
}

unsigned long sys_close(int fd){
    file *filp = get_fd(fd);
    if(!filp) return -EBADF;
    CURRENT->handle_array[fd] = NULL;
    return file_put(filp);
}

unsigned long sys_ioctl(int fd, unsigned long request, void *args){
    file *filp = get_fd(fd);
    if(!filp) return -EBADF;
    if(!filp->f_ops->ioctl) return -ENOTTY;
    return filp->f_ops->ioctl(filp->dentry->dir_node, filp, request, (unsigned long)args);
}

unsigned long sys_read(int fd, void *buf, unsigned long count){
    file *filp = get_fd(fd);
    if(!filp || (filp->mode & O_ACCMODE) == O_WONLY) return -EBADF;
    if(filp->dentry->dir_node->attribute == FS_ATTR_DIR) return -EISDIR;
    if(!filp->f_ops->read) return -EINVAL;
    if(filp->dentry->dir_node->attribute == FS_ATTR_FILE)
        return vm_file_io(filp, buf, count, 0);
    if(!verify_area(buf, count)) return -EFAULT;
    return filp->f_ops->read(filp, buf, count, &filp->position);
}

unsigned long sys_write(int fd, void *buf, unsigned long count){
    file *filp = get_fd(fd);
    if(!filp || (filp->mode & O_ACCMODE) == O_RONLY) return -EBADF;
    if(filp->dentry->dir_node->attribute == FS_ATTR_DIR) return -EISDIR;
    if(!filp->f_ops->write) return -EINVAL;
    if(filp->dentry->dir_node->attribute == FS_ATTR_FILE)
        return vm_file_io(filp, buf, count, 1);
    if(!verify_area(buf, count)) return -EFAULT;
    return filp->f_ops->write(filp, buf, count, &filp->position);
}

unsigned long sys_lseek(int fd, long offset, int whence){
    file *filp = get_fd(fd);
    if(!filp) return -EBADF;
    if(whence < 0 || whence >= SEEK_MAX || !filp->f_ops->lseek) return -EINVAL;
    vm_lock();
    long pos = filp->f_ops->lseek(filp, offset, whence);
    vm_unlock();
    return pos;
}

unsigned long sys_fork(){

    struct PtRegs * regs = (struct PtRegs *)CURRENT->thread->rsp0 - 1;

    return do_fork(regs, 0, regs->r11, 0);
}

unsigned long sys_vfork(){

    struct PtRegs * regs = (struct PtRegs *)CURRENT->thread->rsp0 - 1;

    return do_fork(regs, CLONE_FS | CLONE_SIGNAL | CLONE_VM | CLONE_VFORK, regs->r11, 0);
}

unsigned long sys_execve(char *path){
    long len = strnlen_user(path, PAGE_4K_SIZE);
    if(len <= 0 || len >= (long)PAGE_4K_SIZE) return -EINVAL;
    char *name = kmalloc(len + 1, 0);
    if(!name) return -ENOMEM;
    long rc = -EFAULT;
    if(!strncpy_from_user(path, name, len))
        rc = do_execve((struct PtRegs *)CURRENT->thread->rsp0 - 1, name);
    kfree(name);
    return rc;
}

unsigned long sys_brk(unsigned long addr){
    return vm_brk(CURRENT->lmm, addr);
}

unsigned long sys_mmap(unsigned long addr, unsigned long len, unsigned long prot,
                       unsigned long flags, int fd, long offset){
    if(offset < 0) return -EINVAL;
    return vm_map(CURRENT->lmm, get_fd(fd), addr, len, prot, flags, offset);
}

unsigned long sys_munmap(unsigned long addr, unsigned long len){
    return vm_unmap(CURRENT->lmm, addr, len);
}

unsigned long sys_msync(unsigned long addr, unsigned long len, int flags){
    if(flags != MS_SYNC) return -EINVAL;
    return vm_sync(CURRENT->lmm, addr, len);
}

unsigned long sys_exit(int status){ DoExit(status); }

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

    if(strncpy_from_user(filename, path, pathlen)){ kfree(path); return -EFAULT; }

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

struct dir_buffer {
    void *user;
    unsigned long size;
};

static int fill_dentry(void *opaque, char *name, long namelen, long type, long offset){
    struct dir_buffer *dst = opaque;
    dirent header = {.d_offset = offset, .d_namelen = namelen, .d_type = type};
    unsigned long len = sizeof(header) + namelen + 1;
    if(len > dst->size) return -EINVAL;
    if(copy_to_user(&header, dst->user, sizeof(header)) ||
       copy_to_user(name, (char *)dst->user + sizeof(header), namelen + 1)) return -EFAULT;
    return len;
}

unsigned long sys_getdents(int fd, void *buf, long count){
    file *filp = get_fd(fd);
    if(!filp) return -EBADF;
    if(count < 0 || !verify_area(buf, count)) return -EINVAL;
    if(filp->dentry->dir_node->attribute != FS_ATTR_DIR || !filp->f_ops->readdir) return -ENOTDIR;
    struct dir_buffer dst = {.user = buf, .size = count};
    return filp->f_ops->readdir(filp, (char *)&dst, fill_dentry);
}
