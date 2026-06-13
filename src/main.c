#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "hardware/gpio.h"
#include <stdio.h>
#include <string.h>

#include "dfplayer.h"
#include "i2c_lcd.h"
#include "main_shared.h"
#include "web_server.h"

#define CMD_QUEUE_LEN   8
#define LINE_BUF_SIZE   64
#define DEFAULT_VOLUME  20

#define BTN_PREV_PIN    20
#define BTN_NEXT_PIN    21
#define BTN_PLAY_PIN    22
#define MAX_TRACK       99

#define HOLD_MS         2000
#define BTN_POLL_MS     50
#define HOLD_TICKS      (HOLD_MS / BTN_POLL_MS)

// shared state — written only by Core 0 except lcd_state.wifi_connected / .ip
// (Core 1 writes those before Core 0 reads them, so no race in practice)
volatile lcd_state_t lcd_state = {
    .track = 0, .volume = DEFAULT_VOLUME,
    .playing = false, .mode = UI_TRACK,
    .repeat = REPEAT_OFF,
    .wifi_connected = false, .ip = "---",
};

// simple ring-buffer command queue (Core 0 only — no multicore access)
static command_t _cmd_buf[CMD_QUEUE_LEN];
static int _cmd_head = 0, _cmd_tail = 0;

static inline bool cmd_enqueue(command_t cmd) {
    int next = (_cmd_tail + 1) % CMD_QUEUE_LEN;
    if (next == _cmd_head) return false; // full
    _cmd_buf[_cmd_tail] = cmd;
    _cmd_tail = next;
    return true;
}

static inline bool cmd_dequeue(command_t *out) {
    if (_cmd_head == _cmd_tail) return false;
    *out = _cmd_buf[_cmd_head];
    _cmd_head = (_cmd_head + 1) % CMD_QUEUE_LEN;
    return true;
}

// parse USB serial line into command and enqueue
static void parse_and_enqueue(const char *line) {
    command_t cmd = { .type = CMD_UNKNOWN, .arg = 0 };
    int n;

    if      (sscanf(line, "play %d", &n) == 1 && n >= 1 && n <= 99) { cmd.type = CMD_PLAY;       cmd.arg = n; }
    else if (strcmp(line, "stop")       == 0)                        { cmd.type = CMD_STOP; }
    else if (strcmp(line, "next")       == 0)                        { cmd.type = CMD_NEXT; }
    else if (strcmp(line, "prev")       == 0)                        { cmd.type = CMD_PREV; }
    else if (sscanf(line, "vol %d", &n) == 1 && n >= 0 && n <= 30)  { cmd.type = CMD_VOLUME;     cmd.arg = n; }
    else if (strcmp(line, "repeat all") == 0)                        { cmd.type = CMD_REPEAT_ALL; }
    else if (strcmp(line, "repeat one") == 0)                        { cmd.type = CMD_REPEAT_ONE; }
    else if (strcmp(line, "repeat off") == 0)                        { cmd.type = CMD_REPEAT_OFF; }
    else { printf("ERR: unknown command\r\n"); return; }

    cmd_enqueue(cmd);
}

// ── LCD ──────────────────────────────────────────────────────────────────────
static void lcd_update(int tick, int page) {
    lcd_clear_buff_all();

    if (lcd_state.mode == UI_VOLUME) {
        lcd_buff_printf(0, 0, "[ Vol Mode ]");
        lcd_buff_printf(1, 0, "< Vol: %d >", lcd_state.volume);
    } else if (page == 1) {
        if (lcd_state.wifi_connected) {
            lcd_buff_printf(0, 0, "WiFi: OK");
            lcd_buff_printf(1, 0, "%s", lcd_state.ip);
        } else {
            lcd_buff_printf(0, 0, "WiFi: connecting");
            lcd_buff_printf(1, 0, "please wait...");
        }
    } else {
        lcd_buff_printf(0, 0, lcd_state.track > 0 ? "Track: %d" : "MP3 Controller", lcd_state.track);
        const char *rpt = lcd_state.repeat == REPEAT_ALL ? "ALL" :
                          lcd_state.repeat == REPEAT_ONE ? "ONE" : "---";
        const char *pst = lcd_state.paused  ? "Paus" :
                          lcd_state.playing ? "Play" : "Stop";
        lcd_buff_printf(1, 0, "V:%d %s [%s]", lcd_state.volume, pst, rpt);
    }

    put_buff_to_lcd();
}

// ── Buttons ───────────────────────────────────────────────────────────────────
static void buttons_init(void) {
    const uint pins[] = { BTN_PREV_PIN, BTN_NEXT_PIN, BTN_PLAY_PIN };
    for (int i = 0; i < 3; i++) {
        gpio_init(pins[i]);
        gpio_set_dir(pins[i], GPIO_IN);
        gpio_pull_up(pins[i]);
    }
}

static void buttons_poll(bool *last, int *hold, bool *hold_fired, int *selected_track) {
    bool cur[3];
    cur[0] = gpio_get(BTN_PREV_PIN);
    cur[1] = gpio_get(BTN_NEXT_PIN);
    cur[2] = gpio_get(BTN_PLAY_PIN);

    command_t cmd = { .type = CMD_UNKNOWN, .arg = 0 };

    if (lcd_state.mode == UI_TRACK) {
        for (int i = 0; i < 2; i++) {
            if (!cur[i]) {
                hold[i]++;
                if (hold[i] >= HOLD_TICKS && !hold_fired[i]) {
                    hold_fired[i] = true;
                    lcd_state.mode = UI_VOLUME;
                    printf("BTN: enter vol mode\r\n");
                }
            } else {
                hold[i] = 0;
                hold_fired[i] = false;
            }
        }

        if (!cur[0] && last[0] && hold[0] < HOLD_TICKS) {
            *selected_track = (*selected_track <= 1) ? MAX_TRACK : *selected_track - 1;
            lcd_state.track = *selected_track;
            printf("BTN: select track %d\r\n", *selected_track);
        }
        if (!cur[1] && last[1] && hold[1] < HOLD_TICKS) {
            *selected_track = (*selected_track >= MAX_TRACK) ? 1 : *selected_track + 1;
            lcd_state.track = *selected_track;
            printf("BTN: select track %d\r\n", *selected_track);
        }
        if (!cur[2] && last[2]) {
            if (lcd_state.playing) { cmd.type = CMD_STOP; }
            else                   { cmd.type = CMD_PLAY; cmd.arg = *selected_track; }
            cmd_enqueue(cmd);
            printf("BTN: %s\r\n", lcd_state.playing ? "stop" : "play");
        }
    } else {
        hold[0] = hold[1] = 0;
        hold_fired[0] = hold_fired[1] = false;

        if (!cur[0] && last[0]) {
            int v = lcd_state.volume > 0  ? lcd_state.volume - 1 : 0;
            lcd_state.volume = v; cmd.type = CMD_VOLUME; cmd.arg = v;
            cmd_enqueue(cmd); printf("BTN: vol %d\r\n", v);
        }
        if (!cur[1] && last[1]) {
            int v = lcd_state.volume < 30 ? lcd_state.volume + 1 : 30;
            lcd_state.volume = v; cmd.type = CMD_VOLUME; cmd.arg = v;
            cmd_enqueue(cmd); printf("BTN: vol %d\r\n", v);
        }
        if (!cur[2] && last[2]) {
            lcd_state.mode = UI_TRACK;
            printf("BTN: exit vol mode\r\n");
        }
    }

    last[0] = cur[0]; last[1] = cur[1]; last[2] = cur[2];
}

// ── UART RX ───────────────────────────────────────────────────────────────────
static char  _uart_buf[LINE_BUF_SIZE];
static int   _uart_idx = 0;

static void uart_rx_poll(void) {
    int c = getchar_timeout_us(0);
    if (c == PICO_ERROR_TIMEOUT || c == '\r') return;

    if (c == '\n') {
        _uart_buf[_uart_idx] = '\0';
        if (_uart_idx > 0) {
            printf("> %s\r\n", _uart_buf);
            parse_and_enqueue(_uart_buf);
        }
        _uart_idx = 0;
        return;
    }

    if (_uart_idx < LINE_BUF_SIZE - 1)
        _uart_buf[_uart_idx++] = (char)c;
}

// ── DFPlayer ──────────────────────────────────────────────────────────────────
static void dfplayer_run(int *current_track, bool *was_busy, bool *repeat_active) {
    command_t cmd;

    // BUSY pin = LOW when playing OR paused — only track transitions when not paused
    bool is_busy = dfplayer_is_busy();
    if (!*repeat_active && !lcd_state.paused) {
        if (is_busy && !*was_busy) { lcd_state.playing = true;  printf("INFO: playing\r\n"); }
        if (!is_busy && *was_busy) { lcd_state.playing = false; printf("INFO: track finished\r\n"); }
    }
    *was_busy = is_busy;

    // drain all pending commands
    while (cmd_dequeue(&cmd)) {
        switch (cmd.type) {
            case CMD_PLAY:
                *current_track = cmd.arg;
                lcd_state.track = *current_track;
                dfplayer_play((uint16_t)*current_track);
                printf("OK: playing track %d\r\n", *current_track);
                break;
            case CMD_STOP:
                dfplayer_stop();
                lcd_state.playing = false;
                lcd_state.paused  = false;
                printf("OK: stopped\r\n");
                break;
            case CMD_PAUSE:
                dfplayer_pause();
                lcd_state.paused  = true;
                lcd_state.playing = false;
                printf("OK: paused\r\n");
                break;
            case CMD_RESUME:
                dfplayer_resume();
                lcd_state.paused  = false;
                lcd_state.playing = true;
                printf("OK: resumed\r\n");
                break;
            case CMD_NEXT:
                *current_track = (*current_track >= MAX_TRACK) ? 1 : *current_track + 1;
                lcd_state.track = *current_track;
                dfplayer_next();
                printf("OK: next -> track %d\r\n", *current_track);
                break;
            case CMD_PREV:
                *current_track = (*current_track <= 1) ? MAX_TRACK : *current_track - 1;
                lcd_state.track = *current_track;
                dfplayer_prev();
                printf("OK: prev -> track %d\r\n", *current_track);
                break;
            case CMD_VOLUME:
                lcd_state.volume = cmd.arg;
                dfplayer_volume((uint8_t)cmd.arg);
                printf("OK: vol %d\r\n", cmd.arg);
                break;
            case CMD_REPEAT_ALL:
                *repeat_active = true;
                lcd_state.repeat = REPEAT_ALL;
                dfplayer_repeat_all();
                printf("OK: repeat all\r\n");
                break;
            case CMD_REPEAT_ONE:
                *repeat_active = true;
                lcd_state.repeat = REPEAT_ONE;
                dfplayer_repeat_one((uint16_t)*current_track);
                printf("OK: repeat one (track %d)\r\n", *current_track);
                break;
            case CMD_REPEAT_OFF:
                *repeat_active = false;
                lcd_state.repeat = REPEAT_OFF;
                dfplayer_repeat_off();
                printf("OK: repeat off\r\n");
                break;
            default: break;
        }
    }
}

// ── Core 1 FIFO bridge ────────────────────────────────────────────────────────
// Core 1 packs commands as: (type << 16) | (arg & 0xFFFF)
static void fifo_poll(void) {
    while (multicore_fifo_rvalid()) {
        uint32_t raw = multicore_fifo_pop_blocking();
        command_t cmd = {
            .type = (cmd_type_t)(raw >> 16),
            .arg  = (int)(raw & 0xFFFF),
        };
        cmd_enqueue(cmd);
    }
}

// ── main ──────────────────────────────────────────────────────────────────────
int main(void) {
    stdio_init_all();
    printf("BOOT: Core 0 started\r\n");

    lcd_init();
    lcd_clear_buff_all();
    lcd_buff_printf(0, 0, "MP3 Controller");
    lcd_buff_printf(1, 0, "Starting...");
    put_buff_to_lcd();

    dfplayer_init();
    dfplayer_volume(DEFAULT_VOLUME);
    printf("OK: DFPlayer ready, vol %d\r\n", DEFAULT_VOLUME);

    buttons_init();

    // launch Core 1 (owns CYW43 + lwIP — never touch CYW43 from Core 0)
    multicore_launch_core1(core1_wifi_entry);
    printf("BOOT: Core 1 launched\r\n");

    // timing counters
    absolute_time_t next_btn  = make_timeout_time_ms(BTN_POLL_MS);
    absolute_time_t next_lcd  = make_timeout_time_ms(500);

    int  lcd_tick      = 0;
    int  lcd_page      = 0;
    int  selected_track = 1;
    bool btn_last[3]   = { true, true, true };
    int  btn_hold[2]   = { 0, 0 };
    bool btn_fired[2]  = { false, false };
    bool was_busy      = false;
    bool repeat_active = false;
    int  current_track = 1;

    while (1) {
        uart_rx_poll();
        fifo_poll();

        absolute_time_t now = get_absolute_time();

        if (absolute_time_diff_us(now, next_btn) <= 0) {
            next_btn = make_timeout_time_ms(BTN_POLL_MS);
            buttons_poll(btn_last, btn_hold, btn_fired, &selected_track);
        }

        dfplayer_run(&current_track, &was_busy, &repeat_active);

        if (absolute_time_diff_us(now, next_lcd) <= 0) {
            next_lcd = make_timeout_time_ms(500);
            lcd_update(lcd_tick, lcd_page);
            if (lcd_state.mode != UI_VOLUME) {
                lcd_tick++;
                if (lcd_tick >= 6) { lcd_tick = 0; lcd_page = (lcd_page + 1) % 2; }
            }
        }

        sleep_ms(1);
    }
}
