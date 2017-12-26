#ifndef DISPLAY_H
#define DISPLAY_H

int display_init();
void display_shutdown();
void display_tick(int clocks);

void display_cycle_window_mode();
void debug_display();

#endif

