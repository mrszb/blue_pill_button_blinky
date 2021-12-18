/**
 ******************************************************************************
 * @file           : main.c
 * @author         : Auto-generated by STM32CubeIDE
 * @brief          : Main program body
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2021 STMicroelectronics.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 ******************************************************************************
 */

#include <stdint.h>
#include <stdbool.h>

/*
 * Blue Pill
 * STM32F103C8
 *
 * Input
 * Button PC_15
 *
 * Output
 * On BOARD LED PC_13
 *
 */

#define PERIPH_BASE     ((uint32_t) 0x40000000)
#define SYSTICK_BASE    ((uint32_t) 0xE000E010)

#define GPIOC_BASE      (PERIPH_BASE + 0x11000) // GPIOC base address is 0x40011000
#define GPIOD_BASE      (PERIPH_BASE + 0x11400) // GPIOD base address is 0x40011400
#define GPIOE_BASE      (PERIPH_BASE + 0x11800) // GPIOE base address is 0x40011800
#define RCC_BASE        (PERIPH_BASE + 0x21000) //   RCC base address is 0x40021000

#define GPIOC   ((GPIO_type *)  GPIOC_BASE)
#define GPIOD   ((GPIO_type *)  GPIOD_BASE)
#define GPIOE   ((GPIO_type *)  GPIOE_BASE)
#define RCC     ((RCC_type *)     RCC_BASE)
#define SYSTICK ((STK_type *) SYSTICK_BASE)

/*
 * Register Addresses
 */
typedef struct
{
	uint32_t CRL;      /* GPIO port configuration register low,      Address offset: 0x00 */
	uint32_t CRH;      /* GPIO port configuration register high,     Address offset: 0x04 */
	uint32_t IDR;      /* GPIO port input data register,             Address offset: 0x08 */
	uint32_t ODR;      /* GPIO port output data register,            Address offset: 0x0C */
	uint32_t BSRR;     /* GPIO port bit set/reset register,          Address offset: 0x10 */
	uint32_t BRR;      /* GPIO port bit reset register,              Address offset: 0x14 */
	uint32_t LCKR;     /* GPIO port configuration lock register,     Address offset: 0x18 */
} GPIO_type;

typedef struct
{
	uint32_t CR;       /* RCC clock control register,                Address offset: 0x00 */
	uint32_t CFGR;     /* RCC clock configuration register,          Address offset: 0x04 */
	uint32_t CIR;      /* RCC clock interrupt register,              Address offset: 0x08 */
	uint32_t APB2RSTR; /* RCC APB2 peripheral reset register,        Address offset: 0x0C */
	uint32_t APB1RSTR; /* RCC APB1 peripheral reset register,        Address offset: 0x10 */
	uint32_t AHBENR;   /* RCC AHB peripheral clock enable register,  Address offset: 0x14 */
	uint32_t APB2ENR;  /* RCC APB2 peripheral clock enable register, Address offset: 0x18 */
	uint32_t APB1ENR;  /* RCC APB1 peripheral clock enable register, Address offset: 0x1C */
	uint32_t BDCR;     /* RCC backup domain control register,        Address offset: 0x20 */
	uint32_t CSR;      /* RCC control/status register,               Address offset: 0x24 */
	uint32_t AHBRSTR;  /* RCC AHB peripheral clock reset register,   Address offset: 0x28 */
	uint32_t CFGR2;    /* RCC clock configuration register2,         Address offset: 0x2C */
} RCC_type;

typedef struct
{
	uint32_t CSR;      /* SYSTICK control and status register,       Address offset: 0x00 */
	uint32_t RVR;      /* SYSTICK reload value register,             Address offset: 0x04 */
	uint32_t CVR;      /* SYSTICK current value register,            Address offset: 0x08 */
	uint32_t CALIB;    /* SYSTICK calibration value register,        Address offset: 0x0C */
} STK_type;



/*
 * this was replaced with better headers:
 * see memory map, table 3, page 50,51
 */
//#define base_clock 0x40021000
//#define base_gpioc 0x40011000
//
///* clock enable reg */
//#define offset_APB2 0x18
///* port C clock */
//#define IOPCEN (1<<4)
//
//#define offset_GPIOx_CRH 4
//#define offset_GPIOx_ODR 0x0c
//
//uint32_t *apb2 = (uint32_t *) (base_clock + offset_APB2);
//uint32_t *gpioc_h = (uint32_t *) (base_gpioc + offset_GPIOx_CRH);
//uint32_t *gpioc_out = (uint32_t *) (base_gpioc + offset_GPIOx_ODR);


#if !defined(__SOFT_FP__) && defined(__ARM_FP)
  #warning "FPU is not initialized, but the project is compiling for an FPU. Please initialize the FPU before use."
#endif

void init_systick(uint32_t s, uint8_t en);
int main(void);

volatile uint32_t systick_uptime_millis;


//////////////////////////////////////////////////////////
// circular queue holds sampled button values
// using running total and 1/3, 2/3 hysteresis to debounce

#define DebounceQLen  18
#define LowerThr (DebounceQLen/3)
#define UpperThr (DebounceQLen/3)*2

volatile bool DebounceCircularQueue[DebounceQLen],
	*p_debounce;

// debounced value
volatile bool button_pressed;
volatile int running_total;

typedef enum BtnActions {BTN_UP = 1, BTN_DOWN = 2} BtnAction;

typedef struct BtnEvents
{
	BtnAction action;
	uint32_t systick_time;
} BtnEvent;


bool query_btn_event(BtnEvent* aa)
{
	__disable_irq();
	// next version
	__enable_irq();
	return false;
}

//////////////////////////////////////////////////////////
void SysTick_Handler(void){

	// called every 1ms
	systick_uptime_millis ++;

	// toggle debug pin for the scope
	if (systick_uptime_millis & 1) {
		GPIOC->BSRR = 1<<14 ;
	} else {
		GPIOC->BSRR = 1<<(14 + 16);
	}


	// get latest sample
	bool btn_fresh_sample = ((GPIOC->IDR & (1<<15)) == 0);


	// debouncing routine
	// point to last recorded sample, overwrite with new sample ...
	bool btn_oldest_sample = *p_debounce;
	*p_debounce = btn_fresh_sample;

	// rotate pointer forward
	p_debounce ++;
	if (p_debounce >= DebounceCircularQueue + DebounceQLen)
	    p_debounce = DebounceCircularQueue;

	if (btn_fresh_sample != btn_oldest_sample)
	{
		// if sample out not same as sample in we need to update
		// running total
		if (btn_fresh_sample)
			running_total++;
		else
			running_total--;
	}

	if  (button_pressed)
	{
		if (running_total < LowerThr)
			button_pressed = false;
			// BTN UP EVENT
	}
	else
	{
		if (running_total > UpperThr)
			button_pressed = true;
			// BTN DOWN EVENT
	}
}

void init_keyboard(void)
{
	p_debounce = DebounceCircularQueue;
	// assuming DebounceCircularQueue is zeroed out
	//
	running_total = 0;
	button_pressed = false;
}

void init_systick(uint32_t s, uint8_t en)
{
	// 0: AHB/8 -> (1 MHz)
	// 1: Processor clock (AHB) -> (8 MHz)
	SYSTICK->CSR |= 0x00000;   // run at 1 Mhz
	//SYSTICK->CSR |= 0x00004; // run at 8 Mhz

	// Enable callback
	SYSTICK->CSR |= (en << 1);
	// Load the reload value
	SYSTICK->RVR = s;
	// Set the current value to 0
	SYSTICK->CVR = 0;
	// Enable SysTick
	SYSTICK->CSR |= (1 << 0);
}

int main(void)
{
	// Enable GPIOC
	RCC->APB2ENR |= (1 << 4);

	// Pin 15 input with pull-up
	GPIOC->CRH &= 0x0fffffff;
	GPIOC->CRH |= 0x80000000;
	GPIOC->ODR |= 0x80000000; // ?? not sure how to activate internal pull-up

	// Pin 14 output debug (o-scope)
	GPIOC->CRH &= 0xf0ffffff;
	GPIOC->CRH |= 0x03000000;

	// Pin 13 output LED
	GPIOC->CRH &= 0xff0fffff;
	GPIOC->CRH |= 0x00300000;

	init_keyboard();

	// run systick handler each 1000 cycles (1MHZ clock)
	// enable interrupt
	init_systick(1000, 1);

    /* Loop forever */
	for(;;){

		GPIOC->BSRR = (button_pressed ) ?
				1 << 13 : 1 << (13 + 16);

	}
}
