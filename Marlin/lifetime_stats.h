#ifndef LIFETIME_STATS_H
#define LIFETIME_STATS_H

extern unsigned long lifetime_minutes;
extern unsigned long lifetime_print_minutes;
extern unsigned long lifetime_print_centimeters;
extern unsigned long triptime_minutes;
extern unsigned long triptime_print_minutes;
extern unsigned long triptime_print_centimeters;

void lifetime_stats_init();

void lifetime_stats_tick();

void lifetime_stats_print_start();
void lifetime_stats_print_end();

#endif//LIFETIME_STATS_H
