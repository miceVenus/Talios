#ifndef PRINTK_H
#define PRINTK_H

#include <stdint.h>
#include <stdarg.h>

enum COLOR{
    RED     = 0xFF0000,
    WHITE   = 0xFFFFFF,
    BLACK   = 0x000000,
    YELLOW  = 0xFFE820,
    BLUE    = 0x0062FF
};

typedef struct ScreenInfo{
    int XPixelResolution;
    int YPixelResolution;
    int cursorX;
    int cursorY;
    int charWidth;
    int charHeight;

    uint32_t* framebuffer;
}ScreenInfo;

int VsPrintfk(char* buffer, const char* fmt, va_list args);

int ColorPrintfk(int foregroundColor, int backgroundColor, const char* fmt, ...);

int PrintkInit();

void Putchar(char character, uint32_t ForeColor, uint32_t BackColor);

#endif