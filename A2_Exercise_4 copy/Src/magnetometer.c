/*
 * magnetometer.c
 *
 *  Created on: 24 May 2026
 *      Author: declanelliott
 */


#include "magnetometer.h"
#include "I2C.h"
#include "utils.h"
#include "Room.h"
//need math library for heading calculation
#include <math.h>

//initialise magnetometer on I2C bus
void magnetometerInitialisation(void){

	//set magnetometer to continuous mode, enables 10hz readings
	I2CWrite(MAG_ADDRESS, MAGNETOMETER_MODE_REGISTER, MAGNETOMETER_CONTINUOUS_MODE);

	while(I2CBusy());

	//enable block data update to avoid any wrong data read
	I2CWrite(MAG_ADDRESS, MAGNETOMETER_CONFIGURATION_REGISTER_C, MAGNETOMETER_BLOCK_DATA_UPDATE_ENABLE);

	while(I2CBusy());
}

//takes raw data, timestamp, calculated heading and stores in struct
void magnetometerReadingUpdate(magnetometerReading_t* magnetometerDataReading){

	//create a temporary buffer for data - X, Y, Z axis (2 bytes per)
	static uint8_t buffer[6];

	//read XYZ axis readings into buffer
	I2CRead(MAG_ADDRESS, MAGNETOMETER_DATA_START | MAGNETOMETER_AUTO_INCREMENT, buffer, 6);

	//wait until interrupt is triggered for STOPF
	while(I2CBusy());

	//record timestamp into struct
	magnetometerDataReading->timestamp = TIM2->CNT;

	//16 bits (2 bytes) per axis, split into axis_HIGH and axis_LOW byte
	//move both axis_HIGH and axis_LOW into the struct from the buffer
	magnetometerDataReading->xRaw = (int16_t)((buffer[1] << 8) | buffer[0]);
	magnetometerDataReading->yRaw = (int16_t)((buffer[3] << 8) | buffer[2]);
	magnetometerDataReading->zRaw = (int16_t)((buffer[5] << 8) | buffer[4]);

	//calculate heading
	//use x and y raw data, and use tan() to calculate angle

	//heading will be in radians first
	//calibration offset = (max + min)/2. then subtract offset from raw value

	float yDegreeCalculation = (magnetometerDataReading->yRaw) + 12.5;
	float xDegreeCalculation = (magnetometerDataReading->xRaw) + 46.5;

	//angle is tan(y/x)
	//atan checks if x = 0 and accounts so no math errors
	float radians = atan2f(xDegreeCalculation, yDegreeCalculation);

	//convert to degrees
	float radiansToDegrees = radians * 180.0f/ M_PI;

	//degrees cannot be -ve, therefore need to alter if -ve
	if (radiansToDegrees < 0){
		radiansToDegrees += 360.0;
	}

	//store heading in struct
	magnetometerDataReading->heading = radiansToDegrees;
}
