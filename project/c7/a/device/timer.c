#include "timer.h"
#include "io.h"
#include "print.h"

#define IRQ0_FREQUENCY 100
#define INPUT_FREQUENCY 1193180
#define COUNTER_VALUE INPUT_FREQUENCY / IRQ0_FREQUENCY
#define COUNTER_PORT 0x40
#define COUNTER_NO 0
#define COUNTER_MODE 2
#define READ_WRITE_LATCH 3
#define PIT_CONTROL_PORT 0x43

static void frequency_set(uint8_t counter_port, \
                          uint8_t counter_no, \
                          uint8_t rwl, \
                          uint8_t counter_mode, \
                          uint16_t counter_value) {
    outb(PIT_CONTROL_PORT, \
        (uint8_t)(counter_value << 6 | rwl << 4 | counter_mode << 1));
    outb(counter_port, (uint8_t)counter_value);
    outb(counter_port, (uint8_t)counter_value >> 8);
}

void timer_init() {
    put_str("开始初始化timer\n");
    frequency_set(COUNTER_PORT, \
                  COUNTER_NO, \
                  READ_WRITE_LATCH, \
                  COUNTER_MODE, \
                  COUNTER_VALUE);
    put_str("初始化timer完成\n");
}