#ifndef DFPLAYER_H
#define DFPLAYER_H

#include <stdint.h>

void dfplayer_init(void);
void dfplayer_play(uint16_t track);
void dfplayer_stop(void);
void dfplayer_next(void);
void dfplayer_prev(void);
void dfplayer_volume(uint8_t vol);
void dfplayer_repeat_all(void);
void dfplayer_repeat_one(uint16_t track);
void dfplayer_repeat_off(void);

#endif
