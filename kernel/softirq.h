#define TIME_SIRQ   (1 << 0)

typedef struct softirq{
    void (*action)(void *data);
    void *data;
}softirq;

void add_softirq_status(unsigned long status);
unsigned long get_softirq_status();
void register_softirq(int nr, void (*action)(void * data), void *data);
void unregitser_softirq(int nr);
void softirq_init();