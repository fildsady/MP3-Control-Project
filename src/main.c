#include <stdlib.h>

#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "blink.pio.h" // Header ที่ถูกสร้างอัตโนมัติจากไฟล์ .pio

// นิยาม Pin ที่ใช้งาน
#define PIN_0 0

int main()
{
    stdio_init_all();

    PIO pio = pio0; // เลือกใช้ PIO block 0
    uint sm = 0;    // เลือกใช้ State Machine 0
    uint offset = pio_add_program(pio, &pio_blink_program);
    
    // เริ่มต้น PIO ที่ PIN_0
    pio_blink_program_init(pio, sm, offset, PIN_0);

    // คำนวณความเร็ว: 100ms
    // สมมติ Clock RP2350 = 150MHz. เราใช้คำสั่งถ่วงเวลาใน PIO 32 cycles ต่อหนึ่ง Loop เล็ก
    // เราจะส่งค่าเข้าไปใน FIFO เพื่อกำหนดระยะเวลา
    uint32_t delay_val = 400000; // ตัวเลขตัวอย่างเพื่อให้ได้ประมาณ 100ms (ปรับตามความต้องการ)
    pio_sm_put_blocking(pio, sm, delay_val);

    while (1) {
        // CPU ว่าง 100% เพราะ PIO จัดการเรื่องเวลาและการ Toggle ไฟให้เอง
        tight_loop_contents();
    }
}
