#include "../user/fcntl.h"
#include "../user/mman.h"
#include "../user/stdio.h"
#include "../user/unistd.h"
#include <cpuid.h>

__asm__(".pushsection .text.entry,\"ax\"\n jmp main\n .popsection");

enum { max_workers = 32, window = 750000, page = 4096 };

struct shared {
    int ready[max_workers], done[max_workers], start, stop, peer_ready, peer_stop, phase;
    unsigned long counts[max_workers], cpus, peer_count, release_at;
};

static void check(int ok, const char *name){
    if(!ok){
        printf("SCHED FAIL %s\n", name);
        _exit(1);
    }
}

static void wait_for(int *flag, int value){
    while(__atomic_load_n(flag, __ATOMIC_ACQUIRE) != value) __asm__ volatile("pause");
}

static void signal_flag(int *flag, int value){
    __atomic_store_n(flag, value, __ATOMIC_RELEASE);
}

static void spin_work(unsigned long iterations){
    for(volatile unsigned long i = 0; i < iterations; i++) {}
}

static unsigned int apic_id(void){
    unsigned int a, b, c, d;
    if(__get_cpuid_count(0xb, 0, &a, &b, &c, &d) && b) return d;
    __cpuid(1, a, b, c, d);
    return b >> 24;
}

/* Keep a distinct value live in an SSE register across timer preemption. */
static void worker(struct shared *s, int index){
    unsigned long step = index + 1, sum = step, rounds = 0;
    signal_flag(&s->ready[index], 1);
    wait_for(&s->start, 1);
    while(!__atomic_load_n(&s->stop, __ATOMIC_ACQUIRE)){
        unsigned int id = apic_id();
        check(id < 64, "test APIC ID out of range");
        __atomic_fetch_or(&s->cpus, 1UL << id, __ATOMIC_RELAXED);
        unsigned int n = 2048;
        __asm__ volatile("movq %2, %%xmm0\n\t"
                         "movq %0, %%xmm1\n"
                         "1: paddq %%xmm0, %%xmm1\n\t"
                         "dec %1\n\tjnz 1b\n\t"
                         "movq %%xmm1, %0"
                         : "+r"(sum), "+&r"(n) : "r"(step) : "xmm0", "xmm1", "cc");
        rounds++;
        check(sum == step * (1 + 2048 * rounds), "SSE state across preemption");
    }
    s->counts[index] = rounds;
    signal_flag(&s->done[index], 1);
    _exit(0);
}

static void check_workers(struct shared *s, int workers){
    for(int i = 0; i < workers; i++){
        int child = fork();
        check(child >= 0, "fork workers");
        if(!child) worker(s, i);
    }
    for(int i = 0; i < workers; i++) wait_for(&s->ready[i], 1);
    signal_flag(&s->start, 1);
    spin_work(2 * window);
    signal_flag(&s->stop, 1);
    unsigned long least = ~0UL, most = 0;
    for(int i = 0; i < workers; i++){
        wait_for(&s->done[i], 1);
        if(s->counts[i] < least) least = s->counts[i];
        if(s->counts[i] > most) most = s->counts[i];
    }
    unsigned long mask = __atomic_load_n(&s->cpus, __ATOMIC_RELAXED);
    int ncpu = 0;
    for(unsigned long m = mask; m; m &= m - 1) ncpu++;
    printf("SCHED user CPUs %d, APIC mask %X, worker rounds %D..%D\n", ncpu, mask, least, most);
    check(least && most <= 2 * least, "equal-priority CPU time");
}

static unsigned long peer_count(struct shared *s){
    return __atomic_load_n(&s->peer_count, __ATOMIC_RELAXED);
}

static void check_wakeup(struct shared *s){
    int peer = fork();
    check(peer >= 0, "fork competing process");
    if(!peer){
        signal_flag(&s->peer_ready, 1);
        unsigned long count = 0;
        while(!__atomic_load_n(&s->peer_stop, __ATOMIC_ACQUIRE))
            __atomic_store_n(&s->peer_count, ++count, __ATOMIC_RELAXED);
        _exit(0);
    }
    wait_for(&s->peer_ready, 1);
    spin_work(window);
    unsigned long before = peer_count(s);
    spin_work(window);
    unsigned long baseline = peer_count(s) - before;
    check(baseline > 0, "peer progress before sleep");
    s->release_at = peer_count(s) + 8 * baseline;
    int child = vfork();
    if(!child){
        while(peer_count(s) < s->release_at) __asm__ volatile("pause");
        _exit(0);
    }
    check(child > 0, "vfork sleeper");
    before = peer_count(s);
    spin_work(window);
    unsigned long after = peer_count(s) - before;
    signal_flag(&s->peer_stop, 1);
    printf("SCHED peer progress before sleep %D, after wake %D\n", baseline, after);
    check(after >= baseline / 3 && after > 0, "wake monopolized CPU");
}

static void check_shared_writeback(struct shared *s){
    int fd = open("/VMCHECK.DAT", O_RDWR);
    volatile char *data = mmap(0, page, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    check(fd >= 0 && (void *)data != MAP_FAILED && data[0] == 1, "map shared file");
    int child = fork();
    check(child >= 0, "fork file writer");
    if(!child){
        data[0] = 'A';
        signal_flag(&s->phase, 1);
        wait_for(&s->phase, 2);
        data[1] = 'Z';
        signal_flag(&s->phase, 3);
        wait_for(&s->phase, 4);
        _exit(0);
    }
    wait_for(&s->phase, 1);
    check(data[0] == 'A' && !msync((void *)data, page, MS_SYNC), "first sync");
    signal_flag(&s->phase, 2);
    wait_for(&s->phase, 3);
    check(data[1] == 'Z' && !msync((void *)data, page, MS_SYNC), "second sync");
    signal_flag(&s->phase, 4);
    check(!munmap((void *)data, page) && !close(fd), "release shared file");
}

int main(void){
    int fd = open("/KEYBOARD.DEV", O_RONLY);
    unsigned char key = 0;
    check(fd >= 0 && read(fd, &key, 1) == 1 && key == 0x1c && !close(fd), "PS/2 idle wake");
    putstring("SCHED keyboard woke idle reader\n");
    unsigned char cpus;
    fd = open("/CPUS.DAT", O_RONLY);
    check(fd >= 0 && read(fd, &cpus, 1) == 1 && cpus && cpus <= max_workers / 2 && !close(fd),
          "read expected CPU count");
    struct shared *s = mmap(0, page, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    check((void *)s != MAP_FAILED, "shared counters");
    check_workers(s, cpus <= 4 ? 8 : 2 * cpus);
    check_wakeup(s);
    int child = vfork();
    check(child >= 0, "vfork arithmetic test");
    if(!child){
        int dividend = 1, zero = 0;
        __asm__ volatile("cltd\n\tidivl %1" : "+a"(dividend) : "r"(zero) : "edx", "cc");
        check(0, "divide error returned");
    }
    check_shared_writeback(s);
    check(!munmap(s, page), "release counters");
    putstring("SCHED PASS\n");
    _exit(0);
}
