#ifndef TALIOS_MMAN_ABI_H
#define TALIOS_MMAN_ABI_H

#define PROT_NONE       0x00UL
#define PROT_READ       0x01UL
#define PROT_WRITE      0x02UL
#define PROT_EXEC       0x04UL

#define MAP_SHARED      0x01UL
#define MAP_PRIVATE     0x02UL
#define MAP_FIXED       0x10UL
#define MAP_ANONYMOUS   0x20UL
#define MAP_FAILED      ((void *)-1L)

#define MS_SYNC         4

#endif
