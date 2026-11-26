#ifndef SPIN_LOCK
#define SPIN_LOCK

#include "lib.h"

typedef struct SpinLock_T {
    unsigned int lock;
} SpinLock_T;

/* Raw locks are used with local interrupts disabled. They can be handed
 * across a stack switch without charging either task's preemption count. */
static inline void raw_spin_lock(SpinLock_T *lock){
    while(__atomic_exchange_n(&lock->lock, 1, __ATOMIC_ACQUIRE))
        while(__atomic_load_n(&lock->lock, __ATOMIC_RELAXED)) __asm__ volatile("pause");
}

static inline void raw_spin_unlock(SpinLock_T *lock){
    __atomic_store_n(&lock->lock, 0, __ATOMIC_RELEASE);
}

static inline unsigned long spin_lock_irqsave(SpinLock_T *lock){
    unsigned long flags = irq_save();
    raw_spin_lock(lock);
    return flags;
}

static inline void spin_unlock_irqrestore(SpinLock_T *lock, unsigned long flags){
    raw_spin_unlock(lock);
    irq_restore(flags);
}

void spin_lock_init(SpinLock_T *lock);
void spin_lock(SpinLock_T *lock);
void spin_unlock(SpinLock_T *lock);

#endif
