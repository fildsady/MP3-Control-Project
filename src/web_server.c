#include "web_server.h"
#include "lwip/apps/httpd.h"
#include "pico/cyw43_arch.h"
#include "pico/multicore.h"
#include <string.h>
#include <stdio.h>

// Core 1 writes wifi state here before Core 0 reads it — one-time write, safe
extern volatile lcd_state_t lcd_state;

// ── CGI ───────────────────────────────────────────────────────────────────────
// /control?cmd=play&track=3   /control?cmd=stop
// /control?cmd=next           /control?cmd=prev
// /control?cmd=vol&val=20     /control?cmd=repeat_all / repeat_one / repeat_off
static const char *cgi_control(int index, int n_params,
                                char *params[], char *values[]) {
    (void)index;
    command_t cmd = { .type = CMD_UNKNOWN, .arg = 0 };

    for (int i = 0; i < n_params; i++) {
        if (strcmp(params[i], "cmd") == 0) {
            if      (strcmp(values[i], "play")       == 0) cmd.type = CMD_PLAY;
            else if (strcmp(values[i], "stop")       == 0) cmd.type = CMD_STOP;
            else if (strcmp(values[i], "pause")      == 0) cmd.type = CMD_PAUSE;
            else if (strcmp(values[i], "resume")     == 0) cmd.type = CMD_RESUME;
            else if (strcmp(values[i], "next")       == 0) cmd.type = CMD_NEXT;
            else if (strcmp(values[i], "prev")       == 0) cmd.type = CMD_PREV;
            else if (strcmp(values[i], "vol")        == 0) cmd.type = CMD_VOLUME;
            else if (strcmp(values[i], "repeat_all") == 0) cmd.type = CMD_REPEAT_ALL;
            else if (strcmp(values[i], "repeat_one") == 0) cmd.type = CMD_REPEAT_ONE;
            else if (strcmp(values[i], "repeat_off") == 0) cmd.type = CMD_REPEAT_OFF;
        }
        if (strcmp(params[i], "track") == 0) cmd.arg = atoi(values[i]);
        if (strcmp(params[i], "val")   == 0) cmd.arg = atoi(values[i]);
    }

    if (cmd.type != CMD_UNKNOWN) {
        // pack into uint32 and push to Core 0 via FIFO
        uint32_t packed = ((uint32_t)cmd.type << 16) | (uint16_t)cmd.arg;
        multicore_fifo_push_blocking(packed);
    }

    return "/index.shtml";
}

static const tCGI cgi_handlers[] = {
    { "/control", cgi_control },
};

// ── SSI ───────────────────────────────────────────────────────────────────────
static const char *ssi_tags[] = { "track", "vol", "status", "repeat" };

static u16_t ssi_handler(int index, char *insert, int insert_len) {
    switch (index) {
        case 0: snprintf(insert, insert_len, "%d", lcd_state.track);  break;
        case 1: snprintf(insert, insert_len, "%d", lcd_state.volume); break;
        case 2: snprintf(insert, insert_len, "%s",
                    lcd_state.paused  ? "Paused"  :
                    lcd_state.playing ? "Playing" : "Stopped");       break;
        case 3: snprintf(insert, insert_len, "%s",
                    lcd_state.repeat == REPEAT_ALL ? "All" :
                    lcd_state.repeat == REPEAT_ONE ? "One" : "Off");  break;
    }
    return strlen(insert);
}

// ── Core 1 entry ─────────────────────────────────────────────────────────────
// This function runs entirely on Core 1.
// Core 1 owns CYW43 exclusively — Core 0 must NEVER call any cyw43_* function.
void core1_wifi_entry(void) {
    if (cyw43_arch_init()) {
        printf("ERR: WiFi init failed\r\n");
        while (1) tight_loop_contents();
    }

    cyw43_arch_enable_sta_mode();
    printf("WiFi: connecting to %s ...\r\n", WIFI_SSID);

    if (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASS,
                                            CYW43_AUTH_WPA2_AES_PSK, 30000)) {
        printf("ERR: WiFi connect failed\r\n");
        // keep polling so lwIP stays alive (LED blinks slowly to indicate error)
        while (1) {
            cyw43_arch_poll();
            sleep_ms(10);
        }
    }

    const char *ip_str = ip4addr_ntoa(netif_ip4_addr(netif_default));
    printf("WiFi: connected, IP = %s\r\n", ip_str);

    // update shared state — Core 0 reads these in task_lcd
    lcd_state.wifi_connected = true;
    strncpy((char *)lcd_state.ip, ip_str, sizeof(lcd_state.ip) - 1);

    httpd_init();
    http_set_cgi_handlers(cgi_handlers, LWIP_ARRAYSIZE(cgi_handlers));
    http_set_ssi_handler(ssi_handler, ssi_tags, LWIP_ARRAYSIZE(ssi_tags));
    printf("HTTP: server ready\r\n");

    // poll loop — drives lwIP + blinks LED
    bool led = false;
    absolute_time_t next_blink = make_timeout_time_ms(250);

    while (1) {
        cyw43_arch_poll();

        if (absolute_time_diff_us(get_absolute_time(), next_blink) <= 0) {
            led = !led;
            cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, led);
            next_blink = make_timeout_time_ms(250);
        }

        sleep_ms(1);
    }
}
