#ifndef MOUSE_H
#define MOUSE_H

void mouse_init();
void mouse_irq();

extern int mouse_x;
extern int mouse_y;
extern int mouse_buttons;

#endif