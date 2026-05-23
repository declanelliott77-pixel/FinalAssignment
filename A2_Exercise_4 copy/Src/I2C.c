/*
 * I2C.c
 *
 *  Created on: 6 May 2026
 *      Author: declanelliott
 */

#include "I2C.h"
#include <stdint.h>

//constants for initialising I2C
#define I2C_AF4 0b0100
#define I2C_100KHZ 0x00201D2B
#define PIN6_OFFSET (6*4)
#define PIN7_OFFSET (7*4)

//step definitions
#define IDLE_STEP 0
#define SEND_REGISTER_STEP 1
#define SEND_DATA_STEP 2
#define READ_DATA_STEP 3
#define WAIT_FOR_TC_STEP 4
#define DATA_SENT_STEP 5

//global variables with interrupt status register (ISR)
//will be used with interrupts to avoid polling
static uint8_t interruptSlaveAddress; //seven bit address for slave
static uint8_t interruptSlaveRegister; //slave's internal register address

static uint8_t interruptTxData; //data byte to be written

static uint8_t* interruptRxBuffer; //pointer to store slave data
static uint16_t interruptRxBytes; //no. of bytes left to read from slave

static uint8_t I2CState = IDLE_STEP; //current position in steps

//FUNCTIONS

//Communication Process:

//Start Condition: Master pulls SDA low while SCL is high (like saying "attention everyone")
//Address Phase: Master sends 7-bit slave address + read/write bit
//Data Phase: Exchange of information (register addresses, sensor data)
//Stop Condition: Master releases the lines (like saying "conversation over")
//Use PB6/PB7 for communication  via I2C
void I2CInitialise(void){


	//enable GPIOB & I2C1 clocks
	RCC->AHBENR |= RCC_AHBENR_GPIOBEN;
	RCC->APB1ENR |= RCC_APB1ENR_I2C1EN;

	//Set SCL & SDA pins to alternate function mode in MODER
	GPIOB->MODER &= ~(GPIO_MODER_MODER6 | GPIO_MODER_MODER7); //clear bits
	GPIOB->MODER |= (GPIO_MODER_MODER6_1 | GPIO_MODER_MODER7_1); //set bits to AF mode

	//set to open drain configuration to avoid short circuits (e.g. SDA line being pulled both HIGH and LOW at once)
	GPIOB->OTYPER |= (GPIO_OTYPER_OT_6 | GPIO_OTYPER_OT_7);

	//set AF4 for SCL & SDA pins
	//AFR[0] is for pins 0-7
	GPIOB->AFR[0] &= ~((0xF << PIN6_OFFSET) | (0xF << PIN7_OFFSET)); //clear AF bits for PB6, PB7
	GPIOB->AFR[0] |= (I2C_AF4 << PIN6_OFFSET) | (I2C_AF4 << PIN7_OFFSET); // set PB6, PB7 to AF4 (0100)

	//configure to 100kHz interface
	I2C1->CR1 &= ~I2C_CR1_PE; //disable I2C to set 100kHz
	I2C1->TIMINGR = I2C_100KHZ;

	/*
	 * Enable I2C1 interrupts to trigger I2C1_EV_IRQHandler when:
	 * - TXIE -> request  more data to transmit (TXIS)
	 * - RXIE -> when new byte is received (RXNE)
	 * - NACKIE -> if there's an error/no response (NACKF)
	 * - STOPIE -> end of transmission detected, to reset state (STOPF)
	 * - TCIE -> when n bytes are sent (TC)
	 */

	I2C1->CR1 |= (I2C_CR1_TXIE | I2C_CR1_RXIE | I2C_CR1_NACKIE | I2C_CR1_STOPIE | I2C_CR1_TCIE);

	I2C1->CR1 |= I2C_CR1_PE; //reenable I2C

	//enable interrupt in NVIC
	NVIC_EnableIRQ(I2C1_EV_IRQn);

}

void I2CWrite(uint8_t slaveAddress, uint8_t memoryAddress, uint8_t data){

	//consider if I2C is busy with another step
	while (I2CState != IDLE_STEP);

	//set ISR variables
	interruptSlaveAddress = slaveAddress;
	interruptSlaveRegister = memoryAddress;
	interruptTxData = data;
	I2CState = SEND_REGISTER_STEP;

	/* WRITE: tell slave which register want to write to
	 * configuration of CR2 for start condition, address frame, stop condition
	 * shift interruptSlaveAddress left by 1 for R/W bit (W = 0), sends slaveAddress on bus
	 * I2C_CR2_NBYTES_Pos -> configures N bytes sent through left shift (2)
	 * I2C_CR2_START -> start condition that SDA = LOW, SCL = HIGH
	 * I2C_CR2_AUTOEND -> automatic stop condition, N bytes have been sent
	 */

	I2C1->CR2 = (interruptSlaveAddress << 1) | (2 << I2C_CR2_NBYTES_Pos) | I2C_CR2_START | I2C_CR2_AUTOEND;

}

void I2CRead(uint8_t slaveAddress, uint8_t memoryAddress, uint8_t* bufferPointer, uint16_t byteNumber){

	//consider if I2C is busy with another step
	while (I2CState != IDLE_STEP);

	//set ISR variables
	interruptSlaveAddress = slaveAddress;
	interruptSlaveRegister = memoryAddress;
	interruptRxBuffer = bufferPointer;
	interruptRxBytes = byteNumber;
	I2CState = READ_DATA_STEP;

	/* READ: tell slave which register want to read from
	 * configuration of CR2 for start condition, address frame
	 * shift interruptSlaveAddress left by 1 for R/W bit (R = 1), sends slaveAddress on bus
	 * I2C_CR2_NBYTES_Pos -> configures N bytes sent through left shift (1)
	 * I2C_CR2_START -> start condition that SDA = LOW, SCL = HIGH
	 */

	I2C1->CR2 = (interruptSlaveAddress << 1) | (1 << I2C_CR2_NBYTES_Pos) | I2C_CR2_START;

}

void I2C1_EV_EXTI23_IRQHandler(void) {

	//store current status register to find out why interrupt occured
	uint32_t status = I2C1->ISR;

	//TXIE CASE -> hardware needs a byte to send
	if (status & I2C_ISR_TXIS){
		//for either R/W, must send slave register address first
		if (I2CState == SEND_REGISTER_STEP || I2CState == READ_DATA_STEP){
			I2C1->TXDR = interruptSlaveRegister;

			//if W, change step to write interruptTxData to the slave register
			if(I2CState == SEND_REGISTER_STEP){
				I2CState = SEND_DATA_STEP;
			}
			else{
				I2CState = WAIT_FOR_TC_STEP;
			}
		}

		//if register already sent, write interruptTxData to slave register
		else if (I2CState == SEND_DATA_STEP){
			I2C1->TXDR = interruptTxData;
			I2CState = DATA_SENT_STEP;

		}
	}


	//TC CASE -> transfer complete (read mode repeated start)

	/*
	 * Sequel to I2CRead function. flip the bus to R mode.
	 * left shift interruptSlaveAddress by 1
	 * RD_WRN = 1 to flip into read mode (now 7 bit address + R bit)
	 * specify interruptRxBytes no. of bytes to read
	 * trigger repeated start & end
	 */

	if (status & I2C_ISR_TC){
		if (I2CState == WAIT_FOR_TC_STEP){
			I2C1->CR2 = (interruptSlaveAddress << 1) | I2C_CR2_RD_WRN | (interruptRxBytes << I2C_CR2_NBYTES_Pos) | I2C_CR2_START | I2C_CR2_AUTOEND;
		}
	}

	//RXIE CASE -> slave finished sending byte to master
	if (status & I2C_ISR_RXNE){
		//read byte into *interruptRxBuffer
		//increment pointer for next incoming byte
		*interruptRxBuffer++ = I2C1->RXDR;
	}


	//STOPF CASE -> transmission end w/o errors
	if (status & I2C_ISR_STOPF){
		I2C1->ICR |= I2C_ICR_STOPCF; //clear the stop flag (!stop)
		I2CState = IDLE_STEP; //reset bus state
	}

	//NACK CASE -> no response from slave, corruption
	if (status & I2C_ISR_NACKF){
		I2C1->ICR |= I2C_ICR_NACKCF; //clear the error flag
		I2C1->CR2 |= I2C_CR2_STOP; //force stop condition
		I2CState = IDLE_STEP; //reset bus state
	}

}

//check for IDLE_STEP, if I2C in use
uint8_t I2CBusy(void) {

	return (I2CState != IDLE_STEP);

}
