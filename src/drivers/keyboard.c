#include <stdint.h>
#include "drivers/io.h"

#define KBD_DATA 0x60
#define KBD_STATUS 0x64
#define KBD_BUF_SIZE 128

static char kbd_buf[KBD_BUF_SIZE];
static uint32_t kbd_head = 0;
static uint32_t kbd_tail = 0;

static uint8_t shift_pressed = 0;
static uint8_t ctrl_pressed = 0;
static uint8_t alt_pressed = 0;
static uint8_t caps_lock = 0;
static uint8_t num_lock = 0;
static uint8_t scroll_lock = 0;

#define MAX_WAITERS 16
static void* waiters[MAX_WAITERS];
static int waiter_count = 0;

#define KEY_UP    0x80
#define KEY_DOWN  0x81
#define KEY_LEFT  0x82
#define KEY_RIGHT 0x83
#define KEY_HOME  0x84
#define KEY_END   0x85
#define KEY_PGUP  0x86
#define KEY_PGDN  0x87
#define KEY_INS   0x88
#define KEY_DEL   0x89
#define KEY_F1    0x8A
#define KEY_F2    0x8B
#define KEY_F3    0x8C
#define KEY_F4    0x8D
#define KEY_F5    0x8E
#define KEY_F6    0x8F
#define KEY_F7    0x90
#define KEY_F8    0x91
#define KEY_F9    0x92
#define KEY_F10   0x93
#define KEY_F11   0x94
#define KEY_F12   0x95

static void kbd_push(char c);
int kbd_pop(void);

static char keymap_lower[128] = {
    0,    27,   '1',  '2',  '3',  '4',  '5',  '6',
    '7',  '8',  '9',  '0',  '-',  '=',  '\b', '\t',
    'q',  'w',  'e',  'r',  't',  'y',  'u',  'i',
    'o',  'p',  '[',  ']',  '\n', 0,    'a',  's',
    'd',  'f',  'g',  'h',  'j',  'k',  'l',  ';',
    '\'', '`',  0,    '\\', 'z',  'x',  'c',  'v',
    'b',  'n',  'm',  ',',  '.',  '/',  0,    '*',
    0,    ' ',  0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    '7',
    '8',  '9',  '-',  '4',  '5',  '6',  '+',  '1',
    '2',  '3',  '0',  '.',  0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,
};

static char keymap_upper[128] = {
    0,    27,   '!',  '@',  '#',  '$',  '%',  '^',
    '&',  '*',  '(',  ')',  '_',  '+',  '\b', '\t',
    'Q',  'W',  'E',  'R',  'T',  'Y',  'U',  'I',
    'O',  'P',  '{',  '}',  '\n', 0,    'A',  'S',
    'D',  'F',  'G',  'H',  'J',  'K',  'L',  ':',
    '"',  '~',  0,    '|',  'Z',  'X',  'C',  'V',
    'B',  'N',  'M',  '<',  '>',  '?',  0,    '*',
    0,    ' ',  0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    '7',
    '8',  '9',  '-',  '4',  '5',  '6',  '+',  '1',
    '2',  '3',  '0',  '.',  0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,
};

static int extended = 0;

static void update_leds(void) {
    uint8_t led_state = 0;
    
    if (scroll_lock) led_state |= 0x01;
    if (num_lock)    led_state |= 0x02;
    if (caps_lock)   led_state |= 0x04;

    while (inb(KBD_STATUS) & 0x02);
    
    outb(KBD_DATA, 0xED);
    
    while (inb(KBD_STATUS) & 0x02);
    
    outb(KBD_DATA, led_state);
}

void kbd_add_waiter(void* task) {
    if (waiter_count < MAX_WAITERS) {
        waiters[waiter_count++] = task;
    }
}

void kbd_remove_waiter(void* task) {
    for (int i = 0; i < waiter_count; i++) {
        if (waiters[i] == task) {
            for (int j = i; j < waiter_count - 1; j++) {
                waiters[j] = waiters[j + 1];
            }
            waiter_count--;
            return;
        }
    }
}

static void kbd_wakeup_waiters(void) {
    extern void task_wakeup(void* task);
    for (int i = 0; i < waiter_count; i++) {
        task_wakeup(waiters[i]);
    }
    waiter_count = 0;
}

void keyboard_irq(void) {
    uint8_t sc = inb(KBD_DATA);

    if (sc == 0xE0) {
        extended = 1;
        return;
    }

    if (sc & 0x80) {
        sc &= 0x7F;
        
        if (extended) {
            if (sc == 0x1D) ctrl_pressed = 0;
            if (sc == 0x38) alt_pressed = 0;
        } else {
            if (sc == 0x2A || sc == 0x36) shift_pressed = 0;
            if (sc == 0x1D) ctrl_pressed = 0;
            if (sc == 0x38) alt_pressed = 0;
        }
        
        extended = 0;
        return;
    }

    if (extended) {
        switch (sc) {
            case 0x48: kbd_push(KEY_UP); break;
            case 0x50: kbd_push(KEY_DOWN); break;
            case 0x4B: kbd_push(KEY_LEFT); break;
            case 0x4D: kbd_push(KEY_RIGHT); break;
            case 0x47: kbd_push(KEY_HOME); break;
            case 0x4F: kbd_push(KEY_END); break;
            case 0x49: kbd_push(KEY_PGUP); break;
            case 0x51: kbd_push(KEY_PGDN); break;
            case 0x52: kbd_push(KEY_INS); break;
            case 0x53: kbd_push(KEY_DEL); break;
            case 0x1D: ctrl_pressed = 1; break;
            case 0x38: alt_pressed = 1; break;
        }
        extended = 0;
        return;
    }

    if (sc == 0x2A || sc == 0x36) {
        shift_pressed = 1;
        return;
    }
    if (sc == 0x1D) {
        ctrl_pressed = 1;
        return;
    }
    if (sc == 0x38) {
        alt_pressed = 1;
        return;
    }

    if (sc == 0x3A) {
        caps_lock = !caps_lock;
        update_leds();
        return;
    }
    if (sc == 0x45) {
        num_lock = !num_lock;
        update_leds();
        return;
    }
    if (sc == 0x46) {
        scroll_lock = !scroll_lock;
        update_leds();
        return;
    }

    if (sc >= 0x3B && sc <= 0x44) {
        kbd_push(KEY_F1 + (sc - 0x3B));
        return;
    }
    if (sc == 0x57) {
        kbd_push(KEY_F11);
        return;
    }
    if (sc == 0x58) {
        kbd_push(KEY_F12);
        return;
    }

    if (num_lock && sc >= 0x47 && sc <= 0x53) {
        if (sc == 0x47) kbd_push('7');
        else if (sc == 0x48) kbd_push('8');
        else if (sc == 0x49) kbd_push('9');
        else if (sc == 0x4B) kbd_push('4');
        else if (sc == 0x4C) kbd_push('5');
        else if (sc == 0x4D) kbd_push('6');
        else if (sc == 0x4F) kbd_push('1');
        else if (sc == 0x50) kbd_push('2');
        else if (sc == 0x51) kbd_push('3');
        else if (sc == 0x52) kbd_push('0');
        else if (sc == 0x53) kbd_push('.');
        return;
    }

    char c = 0;
    
    int use_upper = shift_pressed;
    
    if (sc >= 0x10 && sc <= 0x19) {
        if (caps_lock) use_upper = !use_upper;
    } else if (sc >= 0x1E && sc <= 0x26) {
        if (caps_lock) use_upper = !use_upper;
    } else if (sc >= 0x2C && sc <= 0x32) {
        if (caps_lock) use_upper = !use_upper;
    }
    
    if (use_upper) {
        c = keymap_upper[sc];
    } else {
        c = keymap_lower[sc];
    }
    
    if (ctrl_pressed && c >= 'a' && c <= 'z') {
        c = c - 'a' + 1;
    } else if (ctrl_pressed && c >= 'A' && c <= 'Z') {
        c = c - 'A' + 1;
    }
    
    if (c) {
        kbd_push(c);
    }
}

static void kbd_push(char c) {
    uint32_t next = (kbd_head + 1) % KBD_BUF_SIZE;
    if (next != kbd_tail) {
        kbd_buf[kbd_head] = c;
        kbd_head = next;
        kbd_wakeup_waiters();
    }
}

int kbd_pop(void) {
    if (kbd_head == kbd_tail) 
        return -1;
    
    char c = kbd_buf[kbd_tail];
    kbd_tail = (kbd_tail + 1) % KBD_BUF_SIZE;
    return c;
}

void kbd_init(void) {
    shift_pressed = 0;
    ctrl_pressed = 0;
    alt_pressed = 0;
    caps_lock = 0;
    num_lock = 1;
    scroll_lock = 0;
    extended = 0;
    waiter_count = 0;
    
    update_leds();
}