#ifndef KMALLOC_H
#define KMALLOC_H

#include <stdint.h>
#include <stddef.h>

void *kmalloc(size_t size);
void *kmalloc_a(size_t size);   

#endif