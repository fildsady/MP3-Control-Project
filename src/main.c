// Bare-metal CYW43 test — NO FreeRTOS
// If LED blinks → board OK, CYW43 OK
// If USB CDC appears → stdio OK
// Tests hardware without any RTOS complexity
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include <stdio.h>

int main(void) {
    stdio_init_all();

    // brief busy-wait so USB CDC has time to enumerate on host
    sleep_ms(2000);
    printf("BARE: main started\r\n");

    if (cyw43_arch_init()) {
        printf("BARE: CYW43 init FAILED\r\n");
        while (1) sleep_ms(1000);
    }
    printf("BARE: CYW43 init OK — blinking LED\r\n");

    int n = 0;
    while (1) {
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
        sleep_ms(500);
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);
        sleep_ms(500);
        printf("BARE: alive %d\r\n", ++n);
    }
}
