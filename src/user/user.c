#include "syscall/syscall.h"

void user_main(void) {
    exec_sys("INIT.ELF");
}
