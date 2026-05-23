#ifndef MAGNETOMETER_H
#define MAGNETOMETER_H

#include <stdint.h>

//header file below from STMF3disco-BSP
#include "stm32f3xx.h"

//magnetometer address
#define MAG_ADDRESS 0x1E


//register addresses
#define MAGNETOMETER_CONFIGURATION_REGISTER_C 0x62//controls BDU, interrupts
#define MAGNETOMETER_MODE_REGISTER 0x60 //controls both ODR and mode
#define MAGNETOMETER_DATA_START 0x68 //address for first magnetometer reading (x-axis HIGH)

//bits to configure
#define MAGNETOMETER_CONTINUOUS_MODE 0x00 //updates at a constant rate (mode), 10hz
#define MAGNETOMETER_BLOCK_DATA_UPDATE_ENABLE 0x10 //BDU
#define MAGNETOMETER_AUTO_INCREMENT 0x80 //increments to read all bytes


//shared structure for magnetometer readings
typedef struct{
	int16_t xRaw;
	int16_t yRaw;
	int16_t zRaw;
	float heading; //the angle to north
	uint32_t timestamp;
} magnetometerReading_t;

//functions
void timeStampInitialisation(void);

void magnetometerInitialisation(void);

void magnetometerReadingUpdate(magnetometerReading_t* magnetometerDataReading);

#endif
