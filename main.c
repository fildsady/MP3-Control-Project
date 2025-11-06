#include <stdio.h>
#include "pico/stdlib.h"

#include "FreeRTOS.h"
#include "task.h"

#define LED_PIN 2

void task_1(void *pvParameters)
{
    while(1)
    {
        printf("Task 1 is running\n");
        for(int i=0;i<20000000;i++);
    } 
}

void task_2(void *pvParameters)
{
    while(1)
    {
        printf("Task 2 is running\n");
        for(int i=0;i<20000000;i++);
    } 
}


int main()
{
    stdio_init_all();

    xTaskCreate(task_1, "LED_Task", 256, NULL, 1, NULL);
    xTaskCreate(task_2, "LED_Task", 256, NULL, 1, NULL);
    vTaskStartScheduler();

    while(1){};
}