#ifndef SPIN_LOCK
#define SPIN_LOCK

#define __LOCK(lock_) \
        __asm__ __volatile__(   \
        "1:             \n\t"   \
        "lock           \n\t"   \
        "incl %0        \n\t"   \
        "cmpl $1, %0    \n\t"   \
        "je 3f          \n\t"   \
        "2:             \n\t"   \
        "pause          \n\t"   \
        "cmpl $1, %0    \n\t"   \
        "jae 2b         \n\t"   \
        "jmp 1b         \n\t"   \
        "3:             \n\t"   \
        :"=m"((lock_)->lock)       \
        :                       \
        :"memory"               \
    );                          \

//      without preempt count this macro is dangeous;

#define __UNLOCK(lock_)          \
        __asm__ __volatile__(   \
            "movl $0, %0    \n\t"\
            :"=m"((lock_)->lock)   \
            :                   \
            :"memory"           \
        );

typedef struct SpinLock_T{
volatile unsigned int lock; // lock : 1 unlock : 0
}SpinLock_T;

void spin_lock_init(SpinLock_T *lock);

// lock which is not allow re-entrying
void spin_lock(SpinLock_T *lock);

void spin_unlock(SpinLock_T *lock);

#endif