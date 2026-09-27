#include "task.h"
#include "spin_lock.h"

#define preempt_disable()               \
        do{                             \
            CURRENT->preempt_count++;   \
        }while (0);                     \

#define preempt_enable()                \
        do{                             \
            CURRENT->preempt_count--;   \
        }while (0);                     \


//      without preempt count this macro is dangeous;
void spin_lock_init(SpinLock_T *lock){
    lock->lock = 0;
}

// lock which is not allow re-entrying
void spin_lock(SpinLock_T *lock){
    preempt_disable();
    __LOCK(lock);
}

void spin_unlock(SpinLock_T *lock){
    __UNLOCK(lock);
    preempt_enable();
}