#ifndef DFPLAYER_H
#define DFPLAYER_H

#include <stdint.h>
#include <stdbool.h>

void dfplayer_init(void);
bool dfplayer_is_busy(void);  // true = playing, false = idle/finished
void dfplayer_play(uint16_t track);
void dfplayer_stop(void);
void dfplayer_pause(void);
void dfplayer_resume(void);
void dfplayer_next(void);
void dfplayer_prev(void);
void dfplayer_volume(uint8_t vol);
void dfplayer_repeat_all(void);
void dfplayer_repeat_one(uint16_t track);
void dfplayer_repeat_off(void);

#endif
