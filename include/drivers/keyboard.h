#ifndef KBD_H
#define KBD_H

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

void kbd_init(void);
int kbd_pop(void);
void kbd_add_waiter(void* task);
void kbd_remove_waiter(void* task);

#endif