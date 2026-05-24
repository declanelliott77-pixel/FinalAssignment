/*
 * joystick.h
 *
 *  Created on: 24 May 2026
 *      Author: declanelliott
 */

#ifndef JOYSTICK_H_
#define JOYSTICK_H_
#include "Room.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>

typedef struct
{
    uint16_t x_raw;
    uint16_t y_raw;

    int16_t x_offset;
    int16_t y_offset;

    uint8_t button_pressed;
} joystick_state_t;

void joystick_init(void);

uint16_t joystick_read_x_raw(void);
uint16_t joystick_read_y_raw(void);

int16_t joystick_read_x_offset(void);
int16_t joystick_read_y_offset(void);

uint8_t joystick_button_pressed(void);

void joystick_read_state(joystick_state_t *state);

#endif


