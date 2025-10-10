#ifndef PRINTK_H
#define PRINTK_H

#include <stdint.h>
#include <stdarg.h>

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