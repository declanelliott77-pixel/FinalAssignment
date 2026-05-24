/*
 * Room.h
 *
 *  Created on: 20 May 2026
 *      Author: declanelliott
 */
#ifndef ROOM_H
#define ROOM_H
#include <stdint.h>
#include "magnetometer.h"
// Board initialisation
void initialise_board(void);

// Timer initialisation
void timer_init2(uint32_t point_at_target);
void timer_init3(void);
void timer_init4(void);

// Timer control
void timer_start(void);
void timer_start_3(void);
void timer_stop(void);
void timer_stop_3(void);
void delay_us(uint32_t microseconds);
// Magnetometer
void magnetometerInitialisation(void);
void magnetometerReadingUpdate(magnetometerReading_t* magnetometerDataReading);

void joystick_control_servo(void);
// LCD
void lcd_init(void);
void lcd_clear(void);
void lcd_set_cursor(int row, int col);
void lcd_print(char *str);

// I2C (declared here so main.c can call it)
void I2CInitialise(void);

#endif

