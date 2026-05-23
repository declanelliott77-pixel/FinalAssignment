#ifndef ACCELEROMETER_H
#define ACCELEROMETER_H

#include <stdint.h>

//header file below from STMF3disco-BSP
#include "stm32f3xx.h"

//accelerometer address
#define ACC_ADDRESS 0x19

//register addresses for accelerometer
#define ACCELEROMETER_CONTROL_REGISTER_1 0X20
#define ACCELEROMETER_CONTROL_REGISTER_4 0x23
#define ACCELEROMETER_DATA_START 0x28 //address for first accelerometer reading

//bits to configure for control register 1
#define ACCELEROMETER_ODR_100HZ 0x50 //bit mask sets ODR to 100hz
#define ACCELEROMETER_NORMAL_MODE 0x00 //sets to regular power mode
#define ACCELEROMETER_X_ENABLE 0x01 //enable x axis (bit 0)
#define ACCELEROMETER_Y_ENABLE 0x02 //enable y axis (bit 1)
#define ACCELEROMETER_Z_ENABLE 0x04  //enable z axis (bit 2)
#define ACCELEROMETER_AUTO_INCREMENT 0x80 //increments to read all bytes

//bits to configure for 2g
#define ACCELEROMETER_2G  0x00
#define ACCELEROMETER_HIGH_RESOLUTION_MODE  0x08

//shared structure for accelerometer readings
typedef struct{
	int16_t xRaw;
	int16_t yRaw;
	int16_t zRaw;
	float roll; //x axis tilt
	float pitch; //y axis tilt
	//assignment only calls for roll and pitch
	uint32_t timeStamp;
} accelerometerReading_t;

//functions
void accelerometerInitialisation(void);
void accelerometerReadingUpdate(accelerometerReading_t* accelerometerDataReading);

#endif
