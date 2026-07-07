#ifndef STRING_H
#define STRING_H



typedef unsigned long size_t;

char *strrvs(char *Str);

void strcpy(char *Src, char *Dst);

void strncpy(char *Src, char *Dst, size_t n);

int strlen(char *Str);

int strcmp(char *str1, char *str2);

void strncat(char *t_str, char *s_str, unsigned long n);

#endif