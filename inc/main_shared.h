#pragma once
#include <stdbool.h>

typedef enum {
    CMD_PLAY, CMD_STOP, CMD_NEXT, CMD_PREV,
    CMD_PAUSE, CMD_RESUME,
    CMD_VOLUME, CMD_REPEAT_ALL, CMD_REPEAT_ONE, CMD_REPEAT_OFF, CMD_UNKNOWN,
} cmd_type_t;

typedef struct { cmd_type_t type; int arg; } command_t;

typedef enum { UI_TRACK, UI_VOLUME } ui_mode_t;
typedef enum { REPEAT_OFF, REPEAT_ALL, REPEAT_ONE } repeat_mode_t;

typedef struct {
    int           track;
    int           volume;
    bool          playing;
    bool          paused;
    ui_mode_t     mode;
    repeat_mode_t repeat;
    bool          wifi_connected;
    char          ip[16];
} lcd_state_t;
