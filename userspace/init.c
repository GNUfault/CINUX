#include "libc/include/stdio.h"
#include "libc/include/unistd.h"

int main() {
    while(1) {
        printf("hi\n");
        usleep(1000);
    }
}
