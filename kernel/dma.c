#include "dma.h"
#include "lib.h"

static unsigned char DMA_PAGE_REGISTERS[] = {0x0, 0x83, 0x81, 0x82, 0x0, 0x8b, 0x89, 0x8a};
static unsigned char DMA_START_ADDRS[] = {0x0, 0x02, 0x04, 0x06, 0x0, 0xc4, 0xc8, 0xcc};
static unsigned char DMA_COUNT_REGISTERS[] = {0x0, 0x03, 0x05, 0x07, 0x0, 0xc6, 0xca, 0xce};

#define DMA_SINGLE_MASK_REGISTER 0x0a // 0 1 2 3 
#define DMA_MULTI_MASK_REGISTER 0x0f // 0 1 2 3 
#define DMA_MODE_REGISTER 0x0b // 0 1 2 3

#define DMA_FLIP_FLOP_RESET_REGISTER 0x0c // 0 1 2 3 
#define DMA_MASTER_RESET_REGISTER 0x0d // 0 1 2 3 
#define DMA_MASK_RESET_REGISTER 0x0e // 0 1 2 3


#define DMA_GET_VICE_PORT(port) ((((port) - 0x0a) << 1) + 0xd4)
#define DMA_GET_PORT(is_vice, port) (is_vice) ? DMA_GET_VICE_PORT((port)) : (port)


/* @param mode MEANS DMA MODE REGISTER VALUE BITS 0 1 DEPENDS ON CHANNEL NUM */
void isa_dma_init(  unsigned char channel_num, unsigned long addr, unsigned long length, unsigned char mode){

    unsigned char is_vice = channel_num < 4? 0 : 1;

    OUT8b(DMA_GET_PORT(is_vice, DMA_SINGLE_MASK_REGISTER), 0x04 | channel_num);

    OUT8b(DMA_GET_PORT(is_vice, DMA_MODE_REGISTER), mode | channel_num);

    OUT8b(DMA_GET_PORT(is_vice, DMA_FLIP_FLOP_RESET_REGISTER), 0xff);

    OUT8b(DMA_START_ADDRS[channel_num], addr & 0xff);
    OUT8b(DMA_START_ADDRS[channel_num], GetBits(addr, 8, 8));

    OUT8b(DMA_PAGE_REGISTERS[channel_num], GetBits(addr, 16, 8));

    OUT8b(DMA_GET_PORT(is_vice, DMA_FLIP_FLOP_RESET_REGISTER), 0xff);

    unsigned int count = length - 1;
    OUT8b(DMA_COUNT_REGISTERS[channel_num], count & 0xff);
    OUT8b(DMA_COUNT_REGISTERS[channel_num], GetBits(count, 8, 8));

    OUT8b(DMA_GET_PORT(is_vice, DMA_SINGLE_MASK_REGISTER), 0x00 | channel_num);

}