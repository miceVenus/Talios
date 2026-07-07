#ifndef STDIO_H
#define STDIO_H

#include <stdarg.h>

#define NULL 0

int putstring(char *string);

int brk(unsigned long brk);

int printf(const char * restrict format, ...);

int sprintf(char * restrict s, const char * restrict format, ...);

int vsprintf(char * restrict s, const char * restrict format, va_list ap);

unsigned char AnalyzeKeyCode(int fd);

#endif