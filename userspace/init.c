#include "libc/include/stdio.h"
#include "libc/include/string.h"
#include "libc/include/unistd.h"

int main() {
    printf("CINUX Keyboard Test\n");

    char buffer[128];
    int counter = 0;
    
    while(1) {
        printf("[%d]: ", counter++);
        
        int i = 0;
        while(i < 127) {
            char c;
            int result = read(0, &c, 1);
            
            if (result > 0) {
                if (c == '\n') {
                    buffer[i] = '\0';
                    putchar('\n');
                    break;
                }
                if (c == '\b' || c == 127) {
                    if (i > 0) {
                        i--;
                        putchar('\b');
                    }
                } else {
                    buffer[i++] = c;
                    putchar(c);
                }
            }
        }
        
        printf("%u\n", (unsigned)strlen(buffer));
    }
    
    return 0;
}