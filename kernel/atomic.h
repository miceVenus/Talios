typedef struct atomic_t{
    volatile long value;
}atomic_t;

inline void atomic_add(atomic_t * atomic, long value){
    __asm__ volatile("lock addq %1, %0   \n\t"
                    :"=m"(atomic->value):"r"(value)
                    :"memory");
}

