#include <stdint.h>
#include <stddef.h>
#include "printk.h"

// void showColorBand(){

//     typedef struct screenInfo {
//         int width;
//         int height;
//         int pitch;
//         int bpp;
//     } screenInfo;

//     screenInfo screen = {1440, 900, 5760, 32};

//     uint32_t    blue_color[3]       =   {0x004249b9, 0x00353a94, 0x00555bc0};

//     uintptr_t   frameBufferAddr     =   0xffff800000a00000;
//     uint32_t*   frameBuffer         =   (uint32_t*)frameBufferAddr;

//     for(int i = 0; i < 3; i++){
//         for(int y = 20 * i; y < 20 * (i + 1); y++){
//             for(int x = 0; x < screen.width; x++){
//                 frameBuffer[y * screen.width + x] = blue_color[i];
//             }
//         }
//     }
    
// }

void main(){
    // showColorBand();
    PrintkInit();
    ColorPrintfk(0xffff, 0x0000, "This is a Test Line\n");
    ColorPrintfk(0xffff, 0x0000, "This is another Test Line %d \n", 1289);
    ColorPrintfk(0xffff, 0x0000, "This is another Test Line %X \n", 45646);
    while (1) {}
}