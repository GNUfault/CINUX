#include <stdint.h>
#include <stdarg.h>
#include <stddef.h>
#include "string.h"
#include "unistd.h"

#define READ  2
#define EOF (-1)

void putchar(char c) {
    write(STDOUT, &c, 1);
}

void puts(const char* s) {
    write(STDOUT, s, strlen(s));
    putchar('\n');
}

static int syscall_read(int fd, char* buf, uint32_t len) {
    int ret;
    asm volatile(
        "int $0x80"
        : "=a"(ret)
        : "a"(READ), "b"(fd), "c"(buf), "d"(len)
        : "memory"
    );
    return ret;
}

int getchar(void) {
    char c;
    while (1) {
        int result = syscall_read(0, &c, 1);
        if (result > 0) {
            return (unsigned char)c;
        }
        usleep(10);
    }
    return EOF;
}

char* fgets(char* str, int n, void* stream) {
    (void)stream;
    
    if (!str || n <= 0) {
        return NULL;
    }
    
    int i = 0;
    while (i < n - 1) {
        int c = getchar();
        
        if (c == EOF) {
            if (i == 0) {
                return NULL;
            }
            break;
        }
        
        if (c == '\n') {
            str[i++] = '\n';
            break;
        }
        
        if (c == '\b' || c == 127) {
            if (i > 0) {
                i--;
                putchar('\b');
            }
            continue;
        }
        
        str[i++] = (char)c;
        putchar((char)c);
    }
    
    str[i] = '\0';
    return str;
}

static void print_num(uint32_t num, int base, int sign) {
    char buf[32];
    int i = 0;
    int is_negative = 0;
    
    if (sign && (int)num < 0) {
        is_negative = 1;
        num = -(int)num;
    }
    
    if (num == 0) {
        buf[i++] = '0';
    } else {
        while (num > 0) {
            int digit = num % base;
            buf[i++] = (digit < 10) ? ('0' + digit) : ('a' + digit - 10);
            num /= base;
        }
    }
    
    if (is_negative) {
        buf[i++] = '-';
    }
    
    for (int j = 0; j < i / 2; j++) {
        char tmp = buf[j];
        buf[j] = buf[i - 1 - j];
        buf[i - 1 - j] = tmp;
    }
    
    buf[i] = '\0';
    write(STDOUT, buf, i);
}

int printf(const char* format, ...) {
    va_list args;
    va_start(args, format);
    
    int count = 0;
    
    while (*format) {
        if (*format == '%') {
            format++;
            switch (*format) {
                case 'd':
                case 'i': {
                    int num = va_arg(args, int);
                    print_num(num, 10, 1);
                    break;
                }
                case 'u': {
                    uint32_t num = va_arg(args, uint32_t);
                    print_num(num, 10, 0);
                    break;
                }
                case 'x': {
                    uint32_t num = va_arg(args, uint32_t);
                    print_num(num, 16, 0);
                    break;
                }
                case 's': {
                    char* s = va_arg(args, char*);
                    if (s) {
                        write(STDOUT, s, strlen(s));
                    } else {
                        write(STDOUT, "(null)", 6);
                    }
                    break;
                }
                case 'c': {
                    char c = (char)va_arg(args, int);
                    putchar(c);
                    break;
                }
                case '%': {
                    putchar('%');
                    break;
                }
                default:
                    putchar('%');
                    putchar(*format);
                    break;
            }
            format++;
        } else {
            putchar(*format);
            format++;
        }
        count++;
    }
    
    va_end(args);
    return count;
}

static void skip_whitespace(void) {
    int c;
    while (1) {
        c = getchar();
        if (c != ' ' && c != '\t' && c != '\n' && c != '\r') {
            break;
        }
    }
}

static int parse_int(int* value) {
    char buf[32];
    int i = 0;
    int c = getchar();
    int negative = 0;
    
    if (c == '-') {
        negative = 1;
        c = getchar();
    } else if (c == '+') {
        c = getchar();
    }
    
    while (c >= '0' && c <= '9' && i < 31) {
        buf[i++] = c;
        putchar(c);
        c = getchar();
    }
    
    if (i == 0) {
        return 0;
    }
    
    buf[i] = '\0';
    
    int result = 0;
    for (int j = 0; j < i; j++) {
        result = result * 10 + (buf[j] - '0');
    }
    
    if (negative) {
        result = -result;
    }
    
    *value = result;
    return 1;
}

static int parse_string(char* str, int max_len) {
    int i = 0;
    int c;
    
    while (i < max_len - 1) {
        c = getchar();
        
        if (c == EOF || c == ' ' || c == '\t' || c == '\n' || c == '\r') {
            break;
        }
        
        str[i++] = c;
        putchar(c);
    }
    
    str[i] = '\0';
    return i > 0 ? 1 : 0;
}

static int parse_char(char* ch) {
    int c = getchar();
    if (c == EOF) {
        return 0;
    }
    *ch = c;
    putchar(c);
    return 1;
}

int scanf(const char* format, ...) {
    va_list args;
    va_start(args, format);
    
    int count = 0;
    
    while (*format) {
        if (*format == '%') {
            format++;
            
            skip_whitespace();
            
            switch (*format) {
                case 'd':
                case 'i': {
                    int* ptr = va_arg(args, int*);
                    if (ptr && parse_int(ptr)) {
                        count++;
                    } else {
                        va_end(args);
                        return count;
                    }
                    break;
                }
                case 's': {
                    char* ptr = va_arg(args, char*);
                    if (ptr && parse_string(ptr, 256)) {
                        count++;
                    } else {
                        va_end(args);
                        return count;
                    }
                    break;
                }
                case 'c': {
                    char* ptr = va_arg(args, char*);
                    if (ptr && parse_char(ptr)) {
                        count++;
                    } else {
                        va_end(args);
                        return count;
                    }
                    break;
                }
                default:
                    break;
            }
            format++;
        } else if (*format == ' ' || *format == '\t' || *format == '\n') {
            format++;
        } else {
            format++;
        }
    }
    
    va_end(args);
    return count;
}