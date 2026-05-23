/*
 * Module4.c
 *
 *  Created on: 6 May 2026
 *      Author: declanelliott
 */

#include "Room.h"
#include <stdbool.h>

#include <stdint.h>
#include "stm32f303xc.h"
#include "magnetometer.h"
#include "I2C.h"
#include "utils.h"
#include <math.h>
static volatile uint32_t servo1_pulse_width = 1500;   // Magnetometer servo (PA8)
static volatile uint32_t servo2_pulse_width = 1500;   // Spring release servo (PA9)
static volatile uint8_t  spring_release_active = 0;
static int page_index = 0;
static int stable_counter = 0;
static int last_direction = -1;   // 0=N,1=E,2=S,3=W
void drive_output_pin(void);
//LCD Messages

// === COOK (NORTH) ===
char *cook_pages[2][2] = {
    { "The Cook page 1", "I was cooking" },
    { "breakfast and",   "saw nothing"   }
};

// === GOVERNESS (EAST) ===
char *gov_pages[2][2] = {
    { "The Governess -", "When john hit" },
    { "the light the",   "room was bloody" }
};

// === BUTLER (SOUTH) ===
char *butler_pages[2][2] = {
    { "The Butler -",    "Heard the scream" },
    { "turned on lights","and then saw him" }
};

// === MAID (WEST) ===
char *maid_pages[2][2] = {
    { "The Maid -",      "I came to wake" },
    { "him.When I saw",  "him I screamed" }
};


void initialise_board(void)
{
    // === Enable Clocks ===
    RCC->AHBENR  |= RCC_AHBENR_GPIOAEN | RCC_AHBENR_GPIOCEN | RCC_AHBENR_GPIOEEN;
    RCC->AHBENR  |= RCC_AHBENR_ADC12EN;
    RCC->APB1ENR |= RCC_APB1ENR_TIM2EN | RCC_APB1ENR_TIM3EN | RCC_APB1ENR_TIM4EN;

    // === PA8 & PA9 as PWM (Alternate Function) for Servos ===
    // This is critical - normal GPIO output won't work for servos
    GPIOA->MODER   &= ~((3U << 16) | (3U << 18));   // Clear mode
    GPIOA->MODER   |=  ((2U << 16) | (2U << 18));   // Alternate Function mode
    GPIOA->AFR[1]  |=  ((1U << 0)  | (1U << 4));    // AF1 = TIM2 for PA8 and PA9

    // === PC13 as Button Input (active low with pull-up) ===
    GPIOC->MODER   &= ~(3U << 26);                  // Input mode
    GPIOC->PUPDR   |=  (1U << 26);                  // Pull-up

//    // === ADC Setup for Joystick (PA0 & PA1) ===
//    ADC1->CR &= ~ADC_CR_ADEN;
//    ADC1->CR |= ADC_CR_ADEN;
//    while (!(ADC1->ISR & ADC_ISR_ADRDY));

    // === Timer Initialisations (keep your functions) ===
    timer_init2(20000);     // Timer 2 - used for periodic magnetometer checks
    timer_init3();          // Likely servo PWM or timing
    timer_init4();

    // === LCD Initialisation ===
//    lcd_init();
//    lcd_clear();
//    lcd_set_cursor(0, 0);
//    lcd_print("Advance to the Final Room");
    I2CInitialise();
    __enable_irq();         // Make sure interrupts are enabled
    magnetometerInitialisation();
    // Start necessary timers



    // === Magnetometer Initialisation ===


}

void turn_on_all_leds(void)
{
    // Enable GPIOE clock
	   // Enable GPIOE clock
	    // === LED Setup on PE8 to PE15 ===
	        RCC->AHBENR |= RCC_AHBENR_GPIOEEN;           // Enable GPIOE clock (already done in initialise_board, but ok to repeat)

	        // Set PE8 to PE15 as outputs (bits 16 to 31 in MODER)
	        GPIOE->MODER &= ~(0xFFFFU << 16);            // Clear bits 16-31
	        GPIOE->MODER |=  (0x5555U << 16);            // 01 = General purpose output

	        // Turn all LEDs ON
	        GPIOE->ODR ^= (0xFFU << 8);                  // Set PE8 to PE15 high
}

void timer_init2(uint32_t point_at_target) { //Func to initiliase Timer 2
	__disable_irq();
    // Save the configuration

    RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;

    // Configure the timer for the desired interval
    //clock speed is 8 mhz NOT 72 Mhz as APB1 prescaler = 0 and it is using SYSClock

    TIM2->PSC = 799;  // Prescaler: 72MHz / 72000 = 1kHz
    //  PSC= 7     → 1 µs tick
     //   PSC= 799   → 0.1 ms tick (10 kHz)
      //   PSC= 65535  → slowest tick for long delays
    // when setting the interval it is how many ticks you wish to miss ie if psc is 799, 10000 ticks is 1 second
    TIM2->ARR = point_at_target;  // Auto-reload value in milliseconds

    TIM2->DIER |= TIM_DIER_UIE;  // Update Interrupt Enable
    // Enable the timer interrupt
    NVIC_EnableIRQ(TIM2_IRQn);

    __enable_irq();
    TIM2->CR1  |= TIM_CR1_CEN;
}

void timer_init3(void) { //timer to initialise 3 for PWM pulses
    //intiialise timer 3
	    // stop and disable interrupt
	__disable_irq();
	    TIM3->CR1  &= ~TIM_CR1_CEN;
	    TIM3->DIER &= ~TIM_DIER_UIE;

	    // configure
	    TIM3->PSC   = 799;                          // e.g., 799 for 0.1 ms tick @ 8 MHz
	    TIM3->ARR   = 199;             // counts 0..ARR 20 ms
	    TIM3->CNT   = 1;
	    TIM3->EGR   = TIM_EGR_UG;                   // load PSC/ARR

	    // clear pending flags/IRQ
	    TIM3->SR   &= ~TIM_SR_UIF;                  // clear update flag
	    NVIC_ClearPendingIRQ(TIM3_IRQn);

	    // enable interrupt and NVIC, then start
	    TIM3->DIER |= TIM_DIER_UIE;                 // enable update interrupt
	    NVIC_EnableIRQ(TIM3_IRQn);
	    __enable_irq();
	    TIM3->CR1  |= TIM_CR1_CEN;                  // start counter

	}
void timer_init4(void) { //timer to initialise 3 for PWM pulses
	    //intiialise timer 3
		    // stop and disable interrupt
		__disable_irq();
		    TIM4->CR1  &= ~TIM_CR1_CEN;
		    TIM4->DIER &= ~TIM_DIER_UIE;

		    // configure
		    TIM4->PSC   = 799;                          // e.g., 799 for 0.1 ms tick @ 8 MHz
		    TIM4->ARR   = 30000;             // counts 0..ARR 20 ms
		    TIM4->CNT   = 1;
		    TIM4->EGR   = TIM_EGR_UG;                   // load PSC/ARR

		    // clear pending flags/IRQ
		    TIM4->SR   &= ~TIM_SR_UIF;                  // clear update flag
		    NVIC_ClearPendingIRQ(TIM4_IRQn);

		    // enable interrupt and NVIC, then start
		    TIM4->DIER |= TIM_DIER_UIE;                 // enable update interrupt
		    NVIC_EnableIRQ(TIM4_IRQn);
		    __enable_irq();
		    TIM4->CR1  |= TIM_CR1_CEN;                  // start counter

		}

//Busy delays for tim 2 and 3
void delay_us(uint32_t microseconds) {
	        uint32_t start = TIM2->CNT;
	        while ((TIM2->CNT - start) < microseconds) {
	            // Wait
	        }
	    }
void delay_us_tim3(uint32_t microseconds_delay) {
	        uint32_t start = TIM3->CNT;
	        while ((TIM3->CNT - start) < microseconds_delay) {
	            // Wait
	        }
	    }
//IRQ handler should calll the callback, which is the alteration of servo
void TIM3_IRQHandler(void) {

    if (TIM3->SR & TIM_SR_UIF) {  // Check Update Interrupt Flag
        TIM3->SR &= ~TIM_SR_UIF;  // Clear the interrupt flag
        // Always read joystick to control Servo 1 (magnetometer positioning)

        joystick_control_servo();
        turn_on_all_leds();
        // Call the callback function directly (simpler approach)
        drive_output_pin();  // Trigger the callback function
    }

}
// ===================== UPDATED TIM2 HANDLER =====================
void TIM2_IRQHandler(void)
{
    if (TIM2->SR & TIM_SR_UIF)
    {
        TIM2->SR &= ~TIM_SR_UIF;


        // Check button + magnetometer for spring release (Servo 2)
        if (!(GPIOC->IDR & (1 << 13)))   // Button pressed
        {
            magnetometerReading_t mag_reading;

            magnetometerReadingUpdate(&mag_reading);

            bool pointing_west =
                (mag_reading.heading >= 225.0f &&
                 mag_reading.heading <= 315.0f);


                if (pointing_west)
                {
                    spring_release_active = 1;     // Activate spring release servo
                    lcd_clear();
                                   lcd_set_cursor(0, 0);
                                   lcd_print("You Fire ...");
                                   lcd_set_cursor(1, 0);
                                   lcd_print("Gotham is Free");

                    GPIOE->ODR |= 0xF000;          // LED feedback
                }
                else
                {
                    spring_release_active = 0;
                    lcd_clear();
                                  lcd_set_cursor(0, 0);
                                  lcd_print("You fire ...");
                                  lcd_set_cursor(1, 0);
                                  lcd_print("GOTHAM BURNS");
                }
            }

        else
        {
            spring_release_active = 0;
            GPIOE->ODR ^= (0xF << 12);      // Toggle PE12,13,14,15
        }
    }
}

void TIM4_IRQHandler(void)
{
    if (TIM4->SR & TIM_SR_UIF)
    {
        TIM4->SR &= ~TIM_SR_UIF;

        magnetometerReading_t mag;
        magnetometerReadingUpdate(&mag);

        int direction;

        // Determine direction with a tolerance window
        if (mag.heading >= 330 || mag.heading < 30)
            direction = 0;   // Cook
        else if (mag.heading >= 60 && mag.heading < 120)
            direction = 1;   // Governess
        else if (mag.heading >= 150 && mag.heading < 210)
            direction = 2;   // Butler
        else if (mag.heading >= 240 && mag.heading < 300)
            direction = 3;   // Maid
        else
            direction = 4;  // Not pointing at any suspect

        // Reset paging if direction changed
        if (direction != last_direction)
        {
            page_index = 0;
            stable_counter = 0;
            last_direction = direction;
        }


            lcd_clear();

            switch(direction)
            {
                case 0: // Cook
                    lcd_set_cursor(0, 0);
                    lcd_print(cook_pages[page_index][0]);
                    lcd_set_cursor(1, 0);
                    lcd_print(cook_pages[page_index][1]);
                    break;

                case 1: // Governess
                    lcd_set_cursor(0, 0);
                    lcd_print(gov_pages[page_index][0]);
                    lcd_set_cursor(1, 0);
                    lcd_print(gov_pages[page_index][1]);
                    break;

                case 2: // Butler
                    lcd_set_cursor(0, 0);
                    lcd_print(butler_pages[page_index][0]);
                    lcd_set_cursor(1, 0);
                    lcd_print(butler_pages[page_index][1]);
                    break;

                case 3: // Maid
                    lcd_set_cursor(0, 0);
                    lcd_print(maid_pages[page_index][0]);
                    lcd_set_cursor(1, 0);
                    lcd_print(maid_pages[page_index][1]);
                    break;
                case 4:
               lcd_clear();
              lcd_set_cursor(0, 0);
                lcd_print("Jo is found dead");
                lcd_set_cursor(1, 0);
                lcd_print("in a shut room");
            }


            // Flip between page 0 and 1
            page_index ^= 1;
        }
    }



// Timer module start
void timer_start(void) {
    TIM2->CR1 |= TIM_CR1_CEN;  // Enable the counter
}
// Timer module start
void timer_start_3(void) {
    TIM3->CR1 |= TIM_CR1_CEN;  // Enable the counter
}
// Timer module stop
void timer_stop(void) {
    TIM2->CR1 &= ~TIM_CR1_CEN;  // Disable the counter
}
void timer_stop_3(void) {
    TIM3->CR1 &= ~TIM_CR1_CEN;  // Disable the counter
}
//movement of servo through PA8- will activate based on magnometer float
void drive_output_pin(void){
	if (spring_release_active)
	    {
	        GPIOA->ODR |= (1 << 9);
	        delay_us(servo2_pulse_width / 100);   // e.g. 1000 or 2000 µs to release
	        GPIOA->ODR &= ~(1 << 9);
	    }
	else{
	//PA8, PA9, PA10 for up to 3 servos using TIM1 channels 1, 2, and 3. Choose which one you want
	GPIOA->MODER |= (1 << 16);  // Set bit 16 for PA8 output mode
	// Turn PA8 ON (set HIGH)
	    GPIOA->ODR |= (1 << 8);     // Set bit 8 to turn PA8 on
	    //servo foes to full clockwise  at 1ms pulse, centre at 1.5ms, anti-clockwise at 2ms
	    // Depending on where we want the servo create a delay = desired pulse length then trigger low
	    delay_us_tim3(servo1_pulse_width / 100); //9 for full lockwise, 1.5ms delay 14, 2ms delay 19
	    // Set PA8 low
	    GPIOA->ODR &= ~(1 << 8);
	    GPIOE->ODR ^= 0xF000;
	}
}



//begin mangetometer code below

//initialise magnetometer on I2C bus
// Safer version - won't hang if nothing is connected
void magnetometerInitialisation(void)
{
    // Add small delays instead of while(I2CBusy()) to prevent hanging
    I2CWrite(MAG_ADDRESS, MAGNETOMETER_MODE_REGISTER, MAGNETOMETER_CONTINUOUS_MODE);
    delay_ms(20);                    // <-- Changed from while(I2CBusy())

    I2CWrite(MAG_ADDRESS, MAGNETOMETER_CONFIGURATION_REGISTER_C, MAGNETOMETER_BLOCK_DATA_UPDATE_ENABLE);
    delay_ms(20);                    // <-- Changed from while(I2CBusy())
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



joystick_t read_joystick(void)
{
    joystick_t joy = {0};

    // Make sure ADC is ready
    if (!(ADC1->CR & ADC_CR_ADEN)) {
        ADC1->CR |= ADC_CR_ADEN;
        while (!(ADC1->ISR & ADC_ISR_ADRDY));
    }

    // Read X (PA0 - Channel 1)
    ADC1->SQR1 = (1 << 6);
    ADC1->CR |= ADC_CR_ADSTART;
    while (!(ADC1->ISR & ADC_ISR_EOC));
    joy.x = ADC1->DR;

    // Read Y (PA1 - Channel 2)
    ADC1->SQR1 = (2 << 6);
    ADC1->CR |= ADC_CR_ADSTART;
    while (!(ADC1->ISR & ADC_ISR_EOC));
    joy.y = ADC1->DR;

    // Button
    joy.button_pressed = !(GPIOC->IDR & (1 << 13));

    return joy;
}
void joystick_control_servo(void)
{
    joystick_t joy = read_joystick();

    // Map X axis to Servo 1 (PA8)
    servo1_pulse_width = 1000 + ((joy.x * 1000) / 4095);

    // Clamp values
    if (servo1_pulse_width < 1000) servo1_pulse_width = 1000;
    if (servo1_pulse_width > 2000) servo1_pulse_width = 2000;
}


//Screen From here on out
static void delay(int t)

{

    for (volatile int i = 0; i < t * 1000; i++);

}



static void pulse_enable(void)

{

    GPIOB->ODR |= (1 << 2);   // E HIGH

    delay(1);

    GPIOB->ODR &= ~(1 << 2);  // E LOW

    delay(1);

}



static void send_nibble(int data)

{

    GPIOB->ODR &= ~(0xF << 10);   // clear PB10–PB13

    GPIOB->ODR |= (data << 10);   // set data

    pulse_enable();

}



static void send_byte(int data, int rs)

{

    if (rs)

        GPIOB->ODR |= (1 << 0);   // RS = 1 (data)

    else

        GPIOB->ODR &= ~(1 << 0);  // RS = 0 (command)



    send_nibble(data >> 4);

    send_nibble(data & 0xF);

}



void lcd_init(void)

{

    RCC->AHBENR |= RCC_AHBENR_GPIOBEN;



    // PB0, PB2, PB10–PB13 as output

    GPIOB->MODER |= (1 << (0*2));

    GPIOB->MODER |= (1 << (2*2));



    for (int i = 10; i <= 13; i++)

        GPIOB->MODER |= (1 << (i*2));



    delay(50);



    send_nibble(0x3);

    delay(5);

    send_nibble(0x3);

    delay(1);

    send_nibble(0x3);

    send_nibble(0x2);



    send_byte(0x28, 0); // 4-bit mode

    send_byte(0x0C, 0); // display ON

    send_byte(0x06, 0); // entry mode

    send_byte(0x01, 0); // clear

}



void lcd_clear(void)

{

    send_byte(0x01, 0);

    delay(2);

}



void lcd_set_cursor(int row, int col)

{

    int addr = (row == 0) ? 0x80 : 0xC0;

    addr += col;

    send_byte(addr, 0);

}



void lcd_print(char *str)

{

    while (*str)

        send_byte(*str++, 1);

}
