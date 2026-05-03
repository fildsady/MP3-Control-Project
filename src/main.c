#include <stdio.h>
#include <stdlib.h>

#include "pico/stdlib.h"
#include "hardware/irq.h"
#include "hardware/timer.h" // เพิ่มบรรทัดนี้

// นิยาม Pin ที่ใช้งาน
#define PIN_0 0
#define PIN_1 1
#define PIN_2 2
//
// เพิ่ม Function Prototypes (การประกาศฟังก์ชัน) ไว้ที่นี่
bool timer_0_callback(struct repeating_timer *t);
bool timer_1_callback(struct repeating_timer *t);
bool timer_2_callback(struct repeating_timer *t);

int main()
{
    stdio_init_all();

    // เริ่มต้นใช้งาน Pins 0, 1, 2
    uint pins[] = {PIN_0, PIN_1, PIN_2};
    for (int i = 0; i < 3; i++) {
        gpio_init(pins[i]);
        gpio_set_dir(pins[i], GPIO_OUT);
    }

    struct repeating_timer timer0, timer1, timer2;

    // สร้าง Timer 1: 100ms -> Pin 0 (ตรวจสอบผลลัพธ์ของ add_repeating_timer_ms)
    if (add_repeating_timer_ms(-100, timer_0_callback, NULL, &timer0)) {
        // ตั้ง Priority (ใช้ TIMER1_IRQ_0 ตามที่คุณแจ้งว่าคอมไพล์ผ่าน)
        irq_set_priority(TIMER1_IRQ_0 + timer0.alarm_id, 0x40);
    }

    // สร้าง Timer 2: 200ms -> Pin 1
    if (add_repeating_timer_ms(-200, timer_1_callback, NULL, &timer1)) {
        irq_set_priority(TIMER1_IRQ_0 + timer1.alarm_id, 0x50);
    }

    // สร้าง Timer 3: 300ms -> Pin 2
    if (add_repeating_timer_ms(-300, timer_2_callback, NULL, &timer2)) {
        irq_set_priority(TIMER1_IRQ_0 + timer2.alarm_id, 0x60);
    }

    while (1) {
        // ในนี้สามารถรันโค้ดอื่นๆ ได้โดยไม่กระทบกับการกระพริบของ LED
        tight_loop_contents(); 
    }
}

// Callback สำหรับ Pin 0 (100ms)
bool timer_0_callback(struct repeating_timer *t) {
    gpio_put(PIN_0, !gpio_get(PIN_0));
    return true;
}

// Callback สำหรับ Pin 1 (200ms)
bool timer_1_callback(struct repeating_timer *t) {
    gpio_put(PIN_1, !gpio_get(PIN_1));
    return true;
}

// Callback สำหรับ Pin 2 (300ms)
bool timer_2_callback(struct repeating_timer *t) {
    gpio_put(PIN_2, !gpio_get(PIN_2));
    return true;
}