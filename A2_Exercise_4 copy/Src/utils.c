#include "utils.h"
#include "stm32f303xc.h"

void delay_ms(uint32_t ms)
{
    for(uint32_t i = 0; i < ms * 1000; i++) {
        __NOP();
    }
}

uint32_t millis(void)
{
    // Simple implementation - improve later with SysTick if needed
    static uint32_t counter = 0;
    return counter++;
}
