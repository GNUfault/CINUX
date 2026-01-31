#ifndef LIBC_UNISTD_H
#define LIBC_UNISTD_H

#include <stdint.h>

#define STDIN  0
#define STDOUT 1
#define STDERR 2

void usleep(uint32_t ms);
int write(int fd, const char* buf, uint32_t len);

#endif 
