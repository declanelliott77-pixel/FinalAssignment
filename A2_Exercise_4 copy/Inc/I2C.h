#ifndef I2C_H
#define I2C_H

/*
 * This module implements an I2C interface, satisfying the following:
 * -> start condition
 * -> address frame
 * -> ACK/NAK
 * -> data transfer
 * -> stop condition
 *
 * The functions I2CRead and I2CWrite work to initialise these transmissions, an interrupt handler processes the rest to avoid lots of polling
 *
 */

//header file below from STMF3disco-BSP
#include "stm32f3xx.h"

#include <stdint.h>

//initialises I2C-1 on pins PB6 (SCL) & PB7 (SDA), sets 100kHz I2C
void I2CInitialise(void);

//initiates a W transmission to a slave register
void I2CWrite(uint8_t slaveAddress, uint8_t memoryAddress, uint8_t data);

//initiates a R transmission from a slave register
void I2CRead(uint8_t slaveAddress, uint8_t memoryAddress, uint8_t* bufferPointer, uint16_t byteNumber);

// returns 1 if the I2C bus is occupied
uint8_t I2CBusy(void);

#endif
