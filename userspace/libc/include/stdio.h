#ifndef LIBC_STDIO_H
#define LIBC_STDIO_H

#include <stddef.h>

int printf(const char* format, ...);
int scanf(const char* format, ...);
void putchar(char c);
int getchar(void);
void puts(const char* s);
char* fgets(char* str, int n, void* stream);

#define stdin  ((void*)0)
#define stdout ((void*)1)
#define stderr ((void*)2)

#define EOF (-1)

#endif