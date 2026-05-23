#ifndef SPI_H
#define SPI_H

#include <stdint.h>
#include <stdbool.h>

#define SPI_ALTERNATE_FUNCTION_5 0x05
#define SCK_PIN 5
#define MISO_PIN 6
#define MOSI_PIN 7
#define SPI_BAUD_RATE_PRESCALER_16 (0x3 << 3)
#define SPI_8BIT_DATA (0x7 << 8)


#define GPIO_MODE_MASK 0x03
#define GPIO_MODE_ALTERNATE_FUNCTION 0x02
#define GPIO_ALTERNATE_FUNCTION_MASK 0x0F

//functions
void SPIInitialise(void);

void SPIStartTransfer(uint8_t* tx, uint8_t* rx, uint16_t size);

uint8_t SPIBusy(void);

#endif
