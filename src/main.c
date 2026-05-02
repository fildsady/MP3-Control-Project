#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "arm_math.h"
#include "pico/stdlib.h"

#include "FreeRTOS.h"
#include "task.h"

#define LED_PIN1 0
#define LED_PIN2 1
#ifndef PICO_DEFAULT_LED_PIN
#define PICO_DEFAULT_LED_PIN 25
#endif

// ---------------------------------------------------------
// Task 1: ควบคุมไฟ LED ดวงที่ 1 (เชื่อมต่อที่ขา LED_PIN1)
// ทำหน้าที่กระพริบ LED โดยสว่าง 500 ms และ ดับ 500 ms
// ---------------------------------------------------------
void task_1(void *pvParameters)
{
    // กำหนดค่าเริ่มต้นให้กับขา GPIO และตั้งค่าให้เป็น Output (ส่งสัญญาณออก)
    gpio_init(LED_PIN1);          
    gpio_set_dir(LED_PIN1, GPIO_OUT);

    // ลูปการทำงานหลักของ Task (ทำงานวนซ้ำไม่รู้จบ)
    while(1)
    {
        // สั่งให้ LED ติด (ส่งค่าสถานะ High หรือ 1)
        gpio_put(LED_PIN1, 1);  
        // หน่วงเวลา 500 มิลลิวินาที (แปลงเวลาเป็นหน่วย Tick ของระบบปฏิบัติการ)
        vTaskDelay(500 / portTICK_PERIOD_MS); 
        // สั่งให้ LED ดับ (ส่งค่าสถานะ Low หรือ 0)
        gpio_put(LED_PIN1, 0);  
        // หน่วงเวลา 500 มิลลิวินาที ก่อนวนลูปใหม่
        vTaskDelay(500 / portTICK_PERIOD_MS);
    } 
}

// ---------------------------------------------------------
// Task 2: ควบคุมไฟ LED ดวงที่ 2 (เชื่อมต่อที่ขา LED_PIN2)
// ทำหน้าที่กระพริบ LED โดยสว่าง 200 ms และ ดับ 250 ms
// ---------------------------------------------------------
void task_2(void *pvParameters)
{
     // กำหนดค่าเริ่มต้นให้กับขา GPIO และตั้งค่าให้เป็น Output
     gpio_init(LED_PIN2);          
     gpio_set_dir(LED_PIN2, GPIO_OUT);

    // ลูปการทำงานหลักของ Task
    while(1)
    {
        // สั่งให้ LED ติด
        gpio_put(LED_PIN2, 1);  
        // หน่วงเวลา 200 มิลลิวินาที
        vTaskDelay(200 / portTICK_PERIOD_MS); 
        // สั่งให้ LED ดับ
        gpio_put(LED_PIN2, 0);  
        // หน่วงเวลา 250 มิลลิวินาที ก่อนวนลูปใหม่
        vTaskDelay(250 / portTICK_PERIOD_MS);
    } 
}

// ---------------------------------------------------------
// Task Blink Builtin: ควบคุมไฟ LED ที่ติดมากับบอร์ด (Built-in LED)
// ทำหน้าที่กระพริบ LED โดยสว่าง 100 ms และ ดับ 100 ms
// ---------------------------------------------------------
void task_blink_builtin(void *pvParameters)
{
    // กำหนดค่าเริ่มต้นให้กับขา GPIO ของ Built-in LED และตั้งค่าให้เป็น Output
    gpio_init(PICO_DEFAULT_LED_PIN);          
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);

    // ลูปการทำงานหลักของ Task
    while(1)
    {
        // สั่งให้ LED ติด
        gpio_put(PICO_DEFAULT_LED_PIN, 1);  
        // หน่วงเวลา 100 มิลลิวินาที
        vTaskDelay(100 / portTICK_PERIOD_MS); 
        // สั่งให้ LED ดับ
        gpio_put(PICO_DEFAULT_LED_PIN, 0);  
        // หน่วงเวลา 100 มิลลิวินาที ก่อนวนลูปใหม่
        vTaskDelay(100 / portTICK_PERIOD_MS);
    } 
}

// ---------------------------------------------------------
// Task DSP: ประมวลผลสัญญาณดิจิทัล (Digital Signal Processing)
// ทำหน้าที่: สร้างสัญญาณ Sine Wave จำลอง, ใส่ Noise, และทำ Low-Pass Filter
// ---------------------------------------------------------
#define FILTER_TAPS 5
float filter_buffer[FILTER_TAPS] = {0};
int filter_index = 0;

void dsp_task(void *pvParameters)
{
    float t = 0.0f;
    
    while(1)
    {
        // 1. จำลองการอ่านค่าจากเซนเซอร์ (Sine wave 1Hz + สัญญาณรบกวนแบบสุ่ม)
        float true_signal = sinf(t) * 10.0f;
        float noise = ((float)(rand() % 100) / 100.0f - 0.5f) * 4.0f; // Noise แกว่งช่วง -2.0 ถึง +2.0
        float raw_value = true_signal + noise;

        // 2. กระบวนการ DSP: นำค่าเข้า Moving Average Filter (FIR Filter เบื้องต้น)
        filter_buffer[filter_index] = raw_value;
        filter_index = (filter_index + 1) % FILTER_TAPS;

        float filtered_value = 0.0f;
        // ใช้ฟังก์ชันสำเร็จรูปจากไลบรารี CMSIS-DSP
        arm_mean_f32(filter_buffer, FILTER_TAPS, &filtered_value);

        // 3. แสดงผลลัพธ์ผ่าน UART/USB (ใช้ Serial Plotter ดูเทรนด์กราฟได้)
        printf("Raw: %6.2f | Filtered: %6.2f\n", raw_value, filtered_value);

        t += 0.1f; // เพิ่มเวลาจำลอง

        // หน่วงเวลาการสุ่มข้อมูลที่ 10Hz (100 ms)
        vTaskDelay(100 / portTICK_PERIOD_MS);
    } 
}

// ---------------------------------------------------------
// ฟังก์ชัน main: จุดเริ่มต้นของโปรแกรม
// ทำหน้าที่ตั้งค่าเริ่มต้นของระบบ, สร้าง Task และเริ่มการทำงานของ OS
// ---------------------------------------------------------
int main()
{

    stdio_init_all();

    xTaskCreate(task_1, "LED_Task_1", 256, NULL, 1, NULL);
    xTaskCreate(task_2, "LED_Task_2", 256, NULL, 1, NULL);
    xTaskCreate(task_blink_builtin, "Blink_Builtin_Task", 256, NULL, 1, NULL);

    xTaskCreate(dsp_task, "DSP_Task", 1024, NULL, 2, NULL);

    vTaskStartScheduler();

    while(1){};
}