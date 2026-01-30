#include <stdint.h>

static inline int syscall(int num, int arg1, int arg2, int arg3) {
    int ret;
    asm volatile(
        "int $0x80"
        : "=a"(ret)
        : "a"(num), "b"(arg1), "c"(arg2), "d"(arg3)
        : "memory"
    );
    return ret;
}

void usleep(uint32_t ms) {
    syscall(7, ms, 0, 0);
}

int write(int fd, const char* buf, uint32_t len) {
    return syscall(1, fd, (int)buf, len);
}