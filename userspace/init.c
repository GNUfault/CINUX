#include "libc/include/stdio.h"
#include "libc/include/unistd.h"

int main(void) {
    while(1) {
        printf("Hello, world!\n");
        usleep(1000);
    }
}
