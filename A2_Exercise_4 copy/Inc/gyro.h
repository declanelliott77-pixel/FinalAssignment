#ifndef GYRO_H
#define GYRO_H

#include <stdint.h>

#define GYRO_IDENTIFICATION 0X0F
#define GYRO_CONTROL_REGISTER_1 0X20
#define GYRO_CONTROL_REGISTER_2 0X21
#define GYRO_CONTROL_REGISTER_3 0X22
#define GYRO_CONTROL_REGISTER_4 0X23
#define GYRO_CONTROL_REGISTER_5 0X24
#define GYRO_STATUS_REGISTER 0X27

//output data registers (ODRs)
#define GYRO_DATA_START 0X28

//other constants
#define GYRO_READ_BIT 0X80
#define GYRO_AUTO_INCREMENT 0X40
#define GYRO_NORMAL_CONFIGURATION 0X0F
#define GYRO_250_DPS 0X00
#define GPIO_MODE_OUTPUT 0x01
#define GYRO_CHIP_SELECT_PIN 3

#define GYRO_SENSITIVITY 0.00875f //for 250dps
#define TIMER_FREQ_MS 1000.0f

typedef struct {
    int16_t xRaw;
    int16_t yRaw;
    int16_t zRaw;
    float roll;
    float pitch;
    uint32_t timestamp;
} gyroReading_t;

void gyroInitialisation(void);
void gyroRequestRead(void);
uint8_t gyroReadingUpdate(gyroReading_t *data);

#endif
