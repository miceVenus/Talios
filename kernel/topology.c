#include <stdint.h>
#include "memory.h"
#include "smp.h"

/* Legacy BIOS on this platform publishes ACPI tables or an Intel MP table.
 * APIC IDs are firmware IDs, not indices into the kernel's per-CPU arrays. */
struct acpi_header {
    char sig[4]; uint32_t len; uint8_t revision, checksum; char oem[26];
} __attribute__((packed));
struct rsdp {
    char sig[8]; uint8_t checksum; char oem[6]; uint8_t revision;
    uint32_t rsdt, len; uint64_t xsdt; uint8_t tail[4];
} __attribute__((packed));
struct mp_pointer {
    char sig[4]; uint32_t config; uint8_t length, rev, checksum, feature[5];
} __attribute__((packed));
struct mp_header {
    char sig[4]; uint16_t len; uint8_t rev, checksum; char oem[20];
    uint32_t oem_table; uint16_t oem_len, entries; uint32_t lapic; uint8_t tail[4];
} __attribute__((packed));

static int signature(const void *p, const char *s, unsigned int n){
    const char *b = p;
    for(unsigned int i = 0; i < n; i++) if(b[i] != s[i]) return 0;
    return 1;
}
static int checksum(const void *p, unsigned int n){
    const uint8_t *b = p;
    uint8_t sum = 0;
    while(n--) sum += *b++;
    return sum == 0;
}

static int mapped(unsigned long phys){
    unsigned long addr = PAGE_OFFSET + phys;
    unsigned long *pt = PHY_TO_VIRT(GetCr3());
    for(int shift = 39; shift >= 12; shift -= 9){
        unsigned long e = pt[(addr >> shift) & 511];
        if(!(e & PEA_PRESENT)) return 0;
        if(shift == 12 || (e & PEA_PAGE_SIZE)) return 1;
        pt = PHY_TO_VIRT(e & 0x000ffffffffff000UL);
    }
    return 0;
}
static void *table_at(unsigned long phys, unsigned long len){
    if(!phys || phys >= (1UL << 46) || !len || len > (1UL << 20)) return NULL;
    for(unsigned long p = phys & ~4095UL; p < phys + len; p += 4096)
        if(!mapped(p)) return NULL;
    return PHY_TO_VIRT(phys);
}
static void add_apic(unsigned int id){
    for(unsigned int i = 0; i < smp_cpu_count; i++) if(smp_apic_ids[i] == id) return;
    if(smp_cpu_count < NR_CPUS) smp_apic_ids[smp_cpu_count++] = id;
}

static struct acpi_header *acpi_table(unsigned long phys){
    struct acpi_header *h = table_at(phys, sizeof(*h));
    if(!h || h->len < sizeof(*h) || !table_at(phys, h->len) || !checksum(h, h->len)) return NULL;
    return h;
}
static int acpi_cpus(const struct rsdp *r){
    if(!checksum(r, 20)) return 0;
    int extended = r->revision >= 2 && r->len == sizeof(*r) && checksum(r, sizeof(*r)) && r->xsdt;
    struct acpi_header *root = acpi_table(extended ? r->xsdt : r->rsdt);
    if(!root || !signature(root, extended ? "XSDT" : "RSDT", 4)) return 0;
    unsigned int width = extended ? 8 : 4;
    for(unsigned int off = sizeof(*root); off + width <= root->len; off += width){
        uint64_t phys = 0;
        memcopy((char *)root + off, &phys, width);
        struct acpi_header *h = acpi_table(phys);
        if(!h || !signature(h, "APIC", 4) || h->len < sizeof(*h) + 8) continue;
        const uint8_t *end = (const uint8_t *)h + h->len;
        const uint8_t *p = (const uint8_t *)(h + 1) + 8;
        while(p + 2 <= end && p[1] >= 2 && p[1] <= end - p){
            uint32_t flags, id;
            if(p[0] == 0 && p[1] >= 8){
                memcopy((void *)(p + 4), &flags, 4);
                if(flags & 1) add_apic(p[3]);
            }else if(p[0] == 9 && p[1] >= 16){
                memcopy((void *)(p + 4), &id, 4);
                memcopy((void *)(p + 8), &flags, 4);
                if(flags & 1) add_apic(id);
            }
            p += p[1];
        }
        return 1;
    }
    return 0;
}

static int mp_cpus(const struct mp_pointer *mp){
    if(mp->length != 1 || !checksum(mp, 16)) return 0;
    struct mp_header *h = table_at(mp->config, sizeof(*h));
    if(!h || !signature(h, "PCMP", 4) || h->len < sizeof(*h) ||
       !table_at(mp->config, h->len) || !checksum(h, h->len)) return 0;
    const uint8_t *p = (const uint8_t *)(h + 1), *end = (const uint8_t *)h + h->len;
    for(unsigned int i = 0; i < h->entries && p < end; i++){
        int size = p[0] == 0 ? 20 : 8;
        if(p[0] > 4 || end - p < size) break;
        if(p[0] == 0 && (p[3] & 1)) add_apic(p[1]);
        p += size;
    }
    return 1;
}

void smp_discover(unsigned int bsp){
    smp_apic_ids[0] = bsp;
    smp_cpu_count = 1;
    unsigned long ebda = (unsigned long)*(uint16_t *)PHY_TO_VIRT(0x40e) << 4;
    unsigned long conventional = (unsigned long)*(uint16_t *)PHY_TO_VIRT(0x413) << 10;
    unsigned long starts[] = {ebda, conventional >= 1024 ? conventional - 1024 : 0, 0xe0000};
    unsigned long lengths[] = {1024, 1024, 0x20000};
    for(int type = 0; type < 2; type++){
        for(int i = 0; i < 3; i++){
            if(!starts[i] || starts[i] + lengths[i] > 0x100000) continue;
            for(unsigned long j = 0; j < lengths[i]; j += 16){
                const void *p = PHY_TO_VIRT(starts[i] + j);
                if(!type && signature(p, "RSD PTR ", 8) && acpi_cpus(p)) return;
                if(type && signature(p, "_MP_", 4) && mp_cpus(p)) return;
            }
        }
    }
}
