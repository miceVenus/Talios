#ifndef ATOMIC_H
#define ATOMIC_H

typedef struct atomic_t{
    volatile long value;
}atomic_t;

// error function 


inline void atomic_add(atomic_t * atomic, long value){
    __asm__ volatile("lock addq %1, %0   \n\t"
                    :"=m"(atomic->value):"r"(value)
                    :"memory");
}

inline void atomic_sub(atomic_t * atomic, long value){
    __asm__ volatile("lock subq %1, %0  \n\t"
                    :"=m"(atomic->value):"r"(value)
                    :"memory");
}
inline void atomic_inc(atomic_t * atomic){
    __asm__ volatile("lock incq %0 \n\t"
                    :"+m"(atomic->value):
                    :"memory");
}

inline void atomic_dec(atomic_t * atomic){
    __asm__ volatile("lock decq %0  \n\t"
                    :"+m"(atomic->value):
                    :"memory");
}

inline long atomic_read(atomic_t * atomic){
    long value;

    __asm__ volatile("movq %1, %0   \n\t"
                    :"=r"(value):"m"(atomic->value)
                    :"memory");

    return value;
}
inline void atomic_write(atomic_t * atomic, long value){
    __asm__ volatile("movq %1, %0  \n\t"
                    :"=m"(atomic->value):"r"(value)
                    :"memory");
}

inline void atomic_set_mask(atomic_t * atomic, long pos){
    __asm__ volatile("lock bts %0, %1  \n\t"
                    :"=m"(atomic->value):"r"(pos)
                    :"memory");
}

inline void atomic_clear_mask(atomic_t * atomic, long pos){
    __asm__ volatile("lock btr %0, %1  \n\t"
                    :"=m"(atomic->value):"r"(pos)
                    :"memory");
}

#endif