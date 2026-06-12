#pragma once
#include "main_shared.h" // lcd_state_t, cmd_type_t, command_t

// WiFi credentials (test only — do not commit to public repo)
#define WIFI_SSID  "IOT_503"
#define WIFI_PASS  "0811399455"

void task_webserver(void *pvParameters);
