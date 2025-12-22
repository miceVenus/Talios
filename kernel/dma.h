#ifndef DMA_H
#define DMA_H

void isa_dma_init(unsigned char channel_num, unsigned long addr, unsigned long length, unsigned char mode);
#endif