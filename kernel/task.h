struct List{
    struct List *prev;
    struct List *next;
};

typedef unsigned long pml4t_t ;

struct LocalMemManager{
    pml4t_t *pgd;

};

struct ThreadStruct{

};

struct TaskStruct{
    volatile long state;
    unsigned long flags;

    struct List             list;
    struct LocalMemManager  mms;
    struct ThreadStruct*    thread;

    unsigned long AddrLimit;

    long pid;
    long counter;
    long signal;
    long priority;
};