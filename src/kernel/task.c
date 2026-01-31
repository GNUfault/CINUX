#include "lib/libc.h"
#include "kernel/task.h"
#include "kernel/memory.h"
#include "cpu/paging.h"
#include "drivers/timer.h"
#include "lib/string.h"

static task_t* current = 0;
static task_t* task_list = 0;
static int next_pid = 1;

__attribute__((naked))
static void task_wrapper(void) {
    __asm__ volatile(
        "pop %%eax\n"
        "call *%%eax\n"
        "ret\n"
        ::: "eax"
    );
}

task_t* task_current(void) {
    return current;
}

void task_init(void) {
    static task_t kernel_task;
    memset(&kernel_task, 0, sizeof(task_t));
    kernel_task.pid = 0;
    kernel_task.state = TASK_RUNNING;
    kernel_task.page_ctx = kernel_page_context;
    kernel_task.next = &kernel_task;
    current = &kernel_task;
    task_list = &kernel_task;
}

task_t* task_create(void (*entry)(void), const char* name) {
    task_t* t = kmalloc(sizeof(task_t));
    memset(t, 0, sizeof(task_t));
    
    uint32_t* stack = kmalloc(4096);
    uint32_t stack_top = (uint32_t)stack + 4096;
    
    stack_top -= sizeof(uint32_t);
    *(uint32_t*)stack_top = (uint32_t)task_exit;

    stack_top -= sizeof(uint32_t);
    *(uint32_t*)stack_top = (uint32_t)entry;
    
    t->esp = stack_top;
    t->ebp = stack_top;
    t->eip = (uint32_t)task_wrapper;
    
    t->state = TASK_RUNNING;
    t->pid = next_pid++;
    strncpy(t->name, name ? name : "task", TASK_NAME_LEN - 1);
    
    t->page_ctx = paging_create_context();
    
    t->next = task_list->next;
    task_list->next = t;
    
    return t;
}

void task_set_current(task_t* t) {
    current = t;
    if (t && t->page_ctx) {
        paging_switch_context(t->page_ctx);
    }
}

void task_exit(void) {
    task_t* cur = task_current();
    cur->state = TASK_ZOMBIE;
    task_schedule();
    for (;;);
}

void task_sleep(uint32_t ms) {
    sleep(ms);
}

void task_wait(void) {
    task_t* cur = task_current();
    cur->state = TASK_WAITING;
    task_schedule();
}

void task_wakeup(void* task) {
    task_t* t = (task_t*)task;
    if (t && t->state == TASK_WAITING) {
        t->state = TASK_RUNNING;
    }
}

task_t* task_get_list(void) {
    return task_list;
}

int task_kill(int pid) {
    task_t* t = task_get_list();
    task_t* start = t;
    do {
        if (t->pid == pid) {
            if (t == task_current())
                task_exit();   
            t->state = TASK_ZOMBIE;
            return 0;
        }
        t = t->next;
    } while (t != start);
    return -1;
}