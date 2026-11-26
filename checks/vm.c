#include "../user/mman.h"
#include "../user/unistd.h"
#include "../user/stdio.h"
#include "../user/stdlib.h"
#include "../user/fcntl.h"

/* A standalone flat init image for a disposable disk with VMCHECK.DAT.
 * Its three pages contain bytes 1, 2 and 3, the last page is 37 bytes. */
__asm__(".pushsection .text.entry,\"ax\"\n jmp main\n .popsection");

enum { page = 4096, rw = PROT_READ | PROT_WRITE };

static void check(int ok, char *name){
    if(!ok){
        putstring("VM FAIL ");
        putstring(name);
        putstring("\n");
        _exit(1);
    }
}

int main(void){
    int probe = open("/VMCHECK.DAT", O_RDWR);
    char token = 0;
    check(probe >= 0 && read(probe, &token, 1) == 1, "open fixture");
    if(token == 'E'){
        unsigned char inherited;
        check(read(probe, &inherited, 1) == 1 && inherited != probe, "fd survives exec");
        char *view = mmap(0, page, PROT_READ, MAP_PRIVATE, inherited, 0);
        check(view != MAP_FAILED && view[0] == 'E', "map inherited fd after exec");
        token = 'X';
        check(lseek(probe, 0, SEEK_SET) == 0 && write(probe, &token, 1) == 1, "child after exec");
        _exit(0);
    }
    close(probe);

    unsigned long heap = brk(0);
    char *left = malloc(96), *mid = malloc(256), *right = malloc(256), *guard = malloc(page);
    check(left && mid && right && guard, "malloc");
    guard[0] = 14;
    free(mid);
    free(right);
    char *joined = malloc(448);
    check(joined == mid && guard[0] == 14, "coalesced heap blocks");
    char *large = malloc(1UL << 29);
    check(large != NULL, "large lazy malloc");
    large[(1UL << 29) - 1] = 21;
    free(large);
    free(guard);
    free(joined);
    free(left);
    check((unsigned long)brk(0) == heap && !malloc(~0UL), "trim heap and reject overflow");
    
    unsigned long end = heap + (1UL << 30);
    check((unsigned long)brk(end) == end, "sparse brk");
    volatile char *h = (void *)heap;
    h[0] = 9;
    h[end - heap - 1] = 7;
    check(brk(~0UL) == -1 && (unsigned long)brk(0) == end, "failed brk");
    check((unsigned long)brk(heap + page) == heap + page, "shrink");
    check((unsigned long)brk(end) == end && h[end - heap - 1] == 0, "zero on regrow");

    char *p = mmap(0, 1UL << 34, rw, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    volatile int *shared = mmap(0, 2 * page, rw, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    check(p != MAP_FAILED && (void *)shared != MAP_FAILED, "anonymous mmap");
    p[0] = 11;
    int fd = open("/VMCHECK.DAT", O_RDWR);
    int fd2 = open("/./../vmcheck.dat", O_RONLY);
    check(fd >= 0 && fd2 >= 0, "fixture");
    int dir = open("/", O_DIRECTORY | O_RONLY);
    check(dir >= 0 && read(dir, &token, 1) == -1 && close(dir) == 0 &&
          open("/VMCHECK.DAT/child", O_RONLY) == -1, "directory access");
    int pid = fork();
    check(pid >= 0, "sparse fork");
    if(!pid){
        check(p[0] == 11 && h[0] == 9, "inherited contents");
        check(read(fd2, p, 1) == 1 && p[0] == 1, "COW from kernel copyout");
        h[0] = 17;
        p[1UL << 33] = 42;
        shared[page / sizeof(int)] = 33;
        int grandchild = fork();
        check(grandchild >= 0, "second fork");
        if(!grandchild){
            h[0] = 29;
            shared[1] = 1;
            _exit(0);
        }
        while(!shared[1]) {}
        check(h[0] == 17, "nested COW");
        shared[0] = 1;
        _exit(0);
    }
    while(!shared[0]) {}
    check(shared[page / sizeof(int)] == 33 && h[0] == 9 && p[0] == 11 &&
          p[1UL << 33] == 0, "parent isolation and shared first touch");
    check(munmap(p, 1UL << 34) == 0, "sparse unmap");

    /* Cumulatively touch more data than fits in the 128 MiB test guest.
     * A surviving VMA keeps the shared object alive throughout the loop. */
    unsigned long span = 160UL << 20, chunk = 256 * page;
    volatile char *stream = mmap(0, span, rw, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    check((void *)stream != MAP_FAILED, "shared stream");
    for(unsigned long offset = 0; offset < span; offset += chunk){
        for(unsigned long j = 0; j < chunk; j += page) stream[offset + j] = 1;
        check(munmap((void *)(stream + offset), chunk) == 0, "release shared pages");
    }

    volatile char *a = mmap(0, 3 * page, rw, MAP_SHARED, fd, 0);
    volatile char *b = mmap(0, 2 * page, PROT_READ, MAP_SHARED, fd2, page);
    char *priv = mmap(0, page, rw, MAP_PRIVATE, fd2, page);
    check((void *)a != MAP_FAILED && (void *)b != MAP_FAILED && priv != MAP_FAILED, "file mmap");
    check(mmap(0, page, rw, MAP_SHARED, fd2, 0) == MAP_FAILED &&
          mmap(0, page, rw, MAP_PRIVATE, fd2, 1) == MAP_FAILED, "mode and offset");
    check(close(fd) == 0 && close(fd2) == 0, "close before first fault");
    check(b[0] == 2 && a[0] == 1 && b[page + 36] == 3 && b[page + 37] == 0, "offset and EOF tail");
    priv[0] = 4;
    a[page] = 5;
    check(b[0] == 5 && priv[0] == 4, "shared visibility and private isolation");
    fd = open("/VMCHECK.DAT", O_RDWR);
    char c = 0;
    check(lseek(fd, page, SEEK_SET) == page && read(fd, &c, 1) == 1 && c == 5, "read coherence");
    c = 6;
    check(lseek(fd, page + 1, SEEK_SET) == page + 1 && write(fd, &c, 1) == 1 &&
          b[1] == 6, "write coherence");
    check(msync((void *)a, 2 * page, MS_SYNC) == 0, "sync");
    a[page + 1] = 8;
    check(msync((void *)b, page, MS_SYNC) == 0 && b[1] == 8, "sync via read-only alias");
    a[page + 2] = 9;
    priv[3] = 99;
    a[2 * page + 100] = 7;
    check(lseek(fd, 0, SEEK_END) == 2 * page + 37, "writes in zero tail preserve size");
    check(munmap((void *)(a + page), page) == 0 && a[2 * page] == 3, "split");
    check(mmap((void *)(a + page), page, rw, MAP_FIXED | MAP_PRIVATE | MAP_ANONYMOUS,
               -1, 0) == (void *)(a + page) && a[page] == 0, "fixed into hole");
    check(mmap((void *)a, page, rw, MAP_FIXED | MAP_PRIVATE | MAP_ANONYMOUS,
               -1, 0) == (void *)a && a[0] == 0, "fixed replacement");
    check(munmap((void *)a, 3 * page) == 0 && munmap((void *)b, 2 * page) == 0 &&
          munmap(priv, page) == 0 && munmap((void *)a, page) == 0, "unmap ranges and holes");
    check(lseek(fd, page, SEEK_SET) == page && read(fd, &c, 1) == 1 && c == 5, "disk writeback");
    check(read(fd, &c, 1) == 1 && c == 8, "second writeback");
    check(read(fd, &c, 1) == 1 && c == 9 && read(fd, &c, 1) == 1 && c == 2,
          "unmap writes back shared changes only");

    volatile char *ro = mmap(0, 2 * page, PROT_READ, MAP_PRIVATE, fd, 2 * page);
    check((void *)ro != MAP_FAILED && ro[0] == 3, "read-only page");
    if(vfork() == 0){
        ro[0] = 99;
        check(0, "read-only write returned");
    }
    check(ro[0] == 3, "parent survives protection fault");
    if(vfork() == 0){
        (void)ro[page];
        check(0, "read beyond file returned");
    }
    volatile char *none = mmap(0, page, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    check((void *)none != MAP_FAILED && read(fd, (void *)none, 1) == -1, "bad kernel copyout");
    if(vfork() == 0){
        (void)*none;
        check(0, "PROT_NONE returned");
    }
    check(munmap((void *)none, page) == 0 && munmap((void *)ro, 2 * page) == 0, "protected unmap");

    int tfd = open("/VMTRUNC.DAT", O_RDWR);
    volatile char *trunc = mmap(0, page, rw, MAP_SHARED, tfd, 0);
    check(tfd >= 0 && (void *)trunc != MAP_FAILED && trunc[0] == 'T', "map before truncate");
    trunc[0] = 'D';
    int fresh = open("/VMTRUNC.DAT", O_RDWR | O_TRUNC);
    check(fresh >= 0 && lseek(tfd, 0, SEEK_END) == 0, "truncate across opens");
    if(vfork() == 0){
        (void)*trunc;
        check(0, "resident page survived truncate");
    }
    token = 'R';
    check(write(fresh, &token, 1) == 1 && trunc[0] == 'R' && trunc[1] == 0, "refault after regrowth");
    check(munmap((void *)trunc, page) == 0 && close(tfd) == 0 && close(fresh) == 0, "truncate cleanup");

    check(execve("/absent.bin") == -1 && h[0] == 9, "failed exec preserves address space");
    char handshake[2] = {'E', fd};
    check(lseek(fd, 0, SEEK_SET) == 0 && write(fd, handshake, sizeof(handshake)) == sizeof(handshake),
          "exec handshake");
    if(vfork() == 0){
        execve("/init.bin");
        check(0, "exec returned");
    }
    do{
        check(lseek(fd, 0, SEEK_SET) == 0 && read(fd, &token, 1) == 1, "wait for exec");
    }while(token != 'X');
    close(fd);
    putstring("VM PASS\n");
    _exit(0);
}
