#pragma once
#include "main_shared.h"

// WiFi credentials (test only — do not commit to public repo)
#define WIFI_SSID  "IOT_503"
#define WIFI_PASS  "0811399455"

// Entry point for Core 1 — call via multicore_launch_core1(core1_wifi_entry)
void core1_wifi_entry(void);
