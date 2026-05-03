#include <stdio.h>
#include <stdlib.h>

#include "pico/stdlib.h"

#include "FreeRTOS.h"
#include "task.h"

#define PICO_DEFAULT_LED_PIN 25

int main()
{
    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);

    stdio_init_all();

    while(1){
        gpio_put(PICO_DEFAULT_LED_PIN,1);
        sleep_ms(500);
        gpio_put(PICO_DEFAULT_LED_PIN,0);
        sleep_ms(500);  

        printf("Hello World My RP2350\n");
    }   
}