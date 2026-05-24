/*
 * joystick.c
 *
 *  Created on: 24 May 2026
 *      Author: declanelliott
 */

#include "joystick.h"
#include "stm32f303xc.h"
#include "Room.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
static volatile uint32_t servo1_pulse_width = 1500;   // Magnetometer servo (PA8)
static volatile uint32_t servo2_pulse_width = 1500;   // Spring release servo (PA9)
/*
 * Pin assumptions:
 *
 *   VRx -> PA0 -> ADC1_IN1
 *   VRy -> PA3 -> ADC1_IN4
 *   SW  -> PA4 -> digital input
 *
 * Joystick should be powered from 3.3 V, not 5 V.
 */

#define JOY_GPIO_PORT          GPIOA

#define JOY_X_PIN              0U
#define JOY_Y_PIN              3U
#define JOY_SW_PIN             4U

#define JOY_ADC_X_CHANNEL      1U
#define JOY_ADC_Y_CHANNEL      4U

#define ADC_MAX_VALUE          4095U
#define JOY_CENTRE_VALUE       2048U

/*
 * Deadzone around joystick centre.
 * This prevents the PTU from drifting when the joystick is released.
 */
#define JOY_DEADZONE           300

static void joystick_gpio_init(void);
static void joystick_adc_init(void);
static uint16_t joystick_adc_read_channel(uint8_t channel);

void joystick_init(void)
{
    joystick_gpio_init();
    joystick_adc_init();
}

static void joystick_gpio_init(void)
{
    /*
     * Enable GPIOA clock.
     */
    RCC->AHBENR |= RCC_AHBENR_GPIOAEN;

    /*
     * PA0 and PA3 as analog inputs.
     * MODER = 11 for analog mode.
     */
    JOY_GPIO_PORT->MODER |= (3U << (JOY_X_PIN * 2U));
    JOY_GPIO_PORT->MODER |= (3U << (JOY_Y_PIN * 2U));

    /*
     * No pull-up or pull-down for analog pins.
     */
    JOY_GPIO_PORT->PUPDR &= ~(3U << (JOY_X_PIN * 2U));
    JOY_GPIO_PORT->PUPDR &= ~(3U << (JOY_Y_PIN * 2U));

    /*
     * PA4 as digital input for joystick switch.
     * MODER = 00 for input mode.
     */
    JOY_GPIO_PORT->MODER &= ~(3U << (JOY_SW_PIN * 2U));

    /*
     * Enable pull-up on PA4.
     * Most joystick buttons connect SW to GND when pressed.
     * Therefore:
     *   not pressed = 1
     *   pressed     = 0
     */
    JOY_GPIO_PORT->PUPDR &= ~(3U << (JOY_SW_PIN * 2U));
    JOY_GPIO_PORT->PUPDR |=  (1U << (JOY_SW_PIN * 2U));
}

static void joystick_adc_init(void)
{
    /*
     * Enable ADC1/ADC2 peripheral clock.
     */
    RCC->AHBENR |= RCC_AHBENR_ADC12EN;

    /*
     * Use synchronous ADC clock derived from AHB clock.
     * CKMODE = 01 gives HCLK/1 on many STM32F3 devices.
     */
#ifdef ADC12_CCR_CKMODE
    ADC12_COMMON->CCR &= ~(3U << 16U);
    ADC12_COMMON->CCR |=  (1U << 16U);
#endif

    /*
     * Make sure ADC is disabled before configuration.
     */
    if ((ADC1->CR & ADC_CR_ADEN) != 0U)
    {
        ADC1->CR |= ADC_CR_ADDIS;
        while ((ADC1->CR & ADC_CR_ADEN) != 0U)
        {
            // Wait until ADC is disabled
        }
    }

    /*
     * Enable ADC voltage regulator.
     * ADVREGEN bits are [29:28].
     * 00 = disabled
     * 01 = enabled
     */
    ADC1->CR &= ~(3U << 28U);
    ADC1->CR |=  (1U << 28U);

    /*
     * Wait for regulator startup.
     */
    delay_us(2);

    /*
     * Calibrate ADC.
     */
    ADC1->CR |= ADC_CR_ADCAL;
    while ((ADC1->CR & ADC_CR_ADCAL) != 0U)
    {
        // Wait for calibration to finish
    }

    /*
     * 12-bit resolution by default.
     * Right-aligned data by default.
     * Single conversion mode.
     */
    ADC1->CFGR = 0U;

    /*
     * Sampling time.
     * Use a relatively long sample time for stable joystick readings.
     *
     * Channel 1 uses SMPR1 SMP1 bits [5:3].
     * Channel 4 uses SMPR1 SMP4 bits [14:12].
     *
     * 111 = longest sample time.
     */
    ADC1->SMPR1 |= (7U << (JOY_ADC_X_CHANNEL * 3U));
    ADC1->SMPR1 |= (7U << (JOY_ADC_Y_CHANNEL * 3U));

    /*
     * Enable ADC.
     */
    ADC1->CR |= ADC_CR_ADEN;

    while ((ADC1->ISR & ADC_ISR_ADRDY) == 0U)
    {
        // Wait until ADC is ready
    }
}

static uint16_t joystick_adc_read_channel(uint8_t channel)
{
    /*
     * Configure regular sequence length = 1 conversion.
     * SQR1 L bits [3:0] = 0000 means 1 conversion.
     * SQ1 bits [10:6] select the channel.
     */
    ADC1->SQR1 &= ~((0xFU << 0U) | (0x1FU << 6U));
    ADC1->SQR1 |=  ((uint32_t)channel << 6U);

    /*
     * Clear end-of-conversion flag.
     */
    ADC1->ISR |= ADC_ISR_EOC;

    /*
     * Start conversion.
     */
    ADC1->CR |= ADC_CR_ADSTART;

    /*
     * Wait for conversion complete.
     */
    while ((ADC1->ISR & ADC_ISR_EOC) == 0U)
    {
        // Wait
    }

    return (uint16_t)(ADC1->DR & 0x0FFFU);
}

uint16_t joystick_read_x_raw(void)
{
    return joystick_adc_read_channel(JOY_ADC_X_CHANNEL);
}

uint16_t joystick_read_y_raw(void)
{
    return joystick_adc_read_channel(JOY_ADC_Y_CHANNEL);
}

int16_t joystick_read_x_offset(void)
{
    uint16_t raw = joystick_read_x_raw();
    int16_t offset = (int16_t)raw - (int16_t)JOY_CENTRE_VALUE;

    if ((offset < JOY_DEADZONE) && (offset > -JOY_DEADZONE))
    {
        offset = 0;
    }

    return offset;
}

int16_t joystick_read_y_offset(void)
{
    uint16_t raw = joystick_read_y_raw();
    int16_t offset = (int16_t)raw - (int16_t)JOY_CENTRE_VALUE;

    if ((offset < JOY_DEADZONE) && (offset > -JOY_DEADZONE))
    {
        offset = 0;
    }

    return offset;
}

uint8_t joystick_button_pressed(void)
{
    /*
     * With pull-up enabled:
     *   IDR bit = 1 means not pressed
     *   IDR bit = 0 means pressed
     */
    if ((JOY_GPIO_PORT->IDR & (1U << JOY_SW_PIN)) == 0U)
    {
        return 1U;
    }

    return 0U;
}

void joystick_read_state(joystick_state_t *state)
{
    if (state == 0)
    {
        return;
    }

    state->x_raw = joystick_read_x_raw();
    state->y_raw = joystick_read_y_raw();
    state->x_offset = (int16_t)state->x_raw - (int16_t)JOY_CENTRE_VALUE;
    state->y_offset = (int16_t)state->y_raw - (int16_t)JOY_CENTRE_VALUE;

    if ((state->x_offset < JOY_DEADZONE) && (state->x_offset > -JOY_DEADZONE))
    {
        state->x_offset = 0;
    }

    if ((state->y_offset < JOY_DEADZONE) && (state->y_offset > -JOY_DEADZONE))
    {
        state->y_offset = 0;
    }

    state->button_pressed = joystick_button_pressed();
}

