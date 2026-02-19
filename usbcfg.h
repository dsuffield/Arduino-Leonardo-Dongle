/************************************************************************************\

  usbcfg.h - ATmega32U4 configuration

  (c) 2008-2020 Copyright Eckler Software

  Author: David Suffield, dsuffiel@ecklersoft.com

  History:

  See makefile.
  
\************************************************************************************/
#ifndef _USBCFG_H
#define _USBCFG_H

#define MAX_NUM_INT             1   // For tracking Alternate Setting

/*
 * MUID = Microchip USB Class ID
 * Used to identify which of the USB classes owns the current
 * session of control transfer over EP0
 */
#define MUID_NULL               0
#define MUID_USB9               1
#define MUID_RT                 2

/** E N D P O I N T S  A L L O C A T I O N **************************/

#define EP0_BUFF_SIZE 8   // 8, 16, 32, or 64
#define RT_EP 1
#define RT_INTF_ID 0x00
#define RT_EP_SIZE 64
#define RT_EP_OUT EP01_OUT
#define RT_EP_IN EP01_IN
#define MAX_EP_NUMBER 1  // EP1

/* EP0 Vendor Setup commands (bRequest). */
enum STEP_CMD
{
   STEP_SET,     /* set step elements, clear state bits, clear running step count */
   STEP_QUERY,   /* query current step and state info */
   STEP_ABORT_SET,   /* set un-synchronized stop */
   STEP_ABORT_CLEAR,   /* clear un-synchronized stop */
   STEP_OUTPUT0_SET,   /* digital only */
   STEP_OUTPUT0_CLEAR,  /* digital only */
   STEP_OUTPUT1_SET,    /* digital only */
   STEP_OUTPUT1_CLEAR,   /* digital only */
   STEP_SYNC_START_SET,  /* set synchronized start */
   STEP_OUTPUT2_SET,     /* digital only */
   STEP_OUTPUT2_CLEAR,   /* digital only */
   STEP_OUTPUT0_MODE,   /* digital | pwm */
   STEP_OUTPUT0_PWM,    /* 0-255 duty cycle inverted */
   STEP_OUTPUT1_MODE,   /* digital | pwm */
   STEP_OUTPUT1_PWM,    /* 0-255 duty cycle non-inverted */
   STEP_INPUT1_MODE,    /* digital | adc */
   STEP_INPUT2_MODE,    /* digital | adc */
   STEP_INPUT3_MODE,    /* digital | adc */
   STEP_ADC_QUERY,     /* query state and adc info */
   STEP_RES1, STEP_RES2, STEP_RES3, STEP_RES4, STEP_RES5,    /* reserved */
   MS_OS_VENDOR_CODE,      /* Microsoft OS vendor request, make it last */
};

/* OUTPUT0_MODE and OUTPUT1_MODE command parameters (wValue). */
enum OUTPUT_MODE_CMD
{
   OUTPUT_DIGITAL,
   OUTPUT_PWM,
};

/* INPUT1_MODE, INPUT2_MODE and INPUT3_MODE command parameters (wValue). */
enum INPUT_MODE_CMD
{
   INPUT_DIGITAL,
   INPUT_ADC,
};

//#define RT_VERSION_CODE 'a' // We use a printable ASCII character for convenience
//#define RT_VERSION_CODE 'b' // release with Microsoft OS descriptors.
//#define RT_VERSION_CODE 'c' // release with xc8 v1.38 compilier, OUTPUT0-1, suspend.
//#define RT_VERSION_CODE 'd' // release with INPUT0 and OUTPUT0 swapped for TIMER1 support. INPUT0 frequency counter.
//define RT_VERSION_CODE 'e' // release with INPUT0 sync_start support.
#define RT_VERSION_CODE 'f' // release with INPUT3, OUTPUT2, PWM and ADC support.

/************************************************************************************
Arduino pro-micro & leodardo pin connections

<DIR BITS>
PD0 | SCL | D3
PD1 |     | D2
PD2 | RX  | D0
PD3 | TX  | D1

<STEP BITS>
PF4 |     | A3
PF5 |     | A2
PF6 |     | A1
PF7 |     | A0

<OUTPUT BITS>
    | ____ |     |
PC6 | OC4A | D5  | OUTPUT0 *
PD7 | OC4D | D6  | OUTPUT1 *
PD4 |      | D4  | OUTPUT2

<INPUT BITS>
PE6 |       | D7  | INPUT0
PB4 | ADC11 | D8  | INPUT1
PB5 | ADC12 | D9  | INPUT2
PB6 | ADC13 | D10 | INPUT3

<LED BIT>
PD5 | TXLED

<BUG LED BIT>
PB0 | RXLED

<UNUSED BITS>
PB3 | MISO | D14 #
PB1 | SCK  | D15 #
PB2 | MOSI | D16 #
PF0 |      | A5 +
PF1 |      | A4 +
PB7 |      | D11 +
PD6 |      | D12 +
PC7 |      | D13 +

(*) = PWM  (#) pro-micro only  (+) = leodardo only
************************************************************************************/

/* 
 * See following register defines at /usr/lib/avr/include/avr/iom32u4.h 
 * See bit macros at /usr/lib/avr/include/avr/sfr_defs.h
 */

#define LED_DDR  DDRD
#define LED_PIN  PD5
#define LED_PORT PORTD
#define LED_FLIP PIND
#define BUG_LED_DDR  DDRB
#define BUG_LED_PIN  PB0
#define BUG_LED_PORT PORTB
#define BUG_LED_FLIP PINB
#define INPUT0_DDR DDRE
#define INPUT0_PIN PE6
#define INPUT0_PORT PINE
#define INPUT0_PUP PORTE
#define INPUT1_DDR DDRB
#define INPUT1_PIN PB4
#define INPUT1_PORT PINB
#define INPUT1_PUP PORTB
#define INPUT1_ADC _BV(REFS0) | _BV(ADLAR) | 0x3  // ADC11 channel select
#define INPUT2_DDR DDRB
#define INPUT2_PIN PB5
#define INPUT2_PORT PINB
#define INPUT2_PUP PORTB
#define INPUT2_ADC _BV(REFS0) | _BV(ADLAR) | 0x4  // ADC12 channel select
#define INPUT3_DDR DDRB
#define INPUT3_PIN PB6
#define INPUT3_PORT PINB
#define INPUT3_PUP PORTB
#define INPUT3_ADC _BV(REFS0) | _BV(ADLAR) | 0x5  // ADC13 channel select
#define OUTPUT0_DDR DDRC
#define OUTPUT0_PIN PC6
#define OUTPUT0_PORT PORTC
#define OUTPUT0_OCR4 OCR4A    // timer4 pwm output compare register
#define OUTPUT1_DDR DDRD
#define OUTPUT1_PIN PD7
#define OUTPUT1_PORT PORTD
#define OUTPUT1_OCR4 OCR4D    // timer4 pwm output compare register
#define OUTPUT2_DDR DDRD
#define OUTPUT2_PIN PD4
#define OUTPUT2_PORT PORTD

/* Step/Dir output bit definitions */
#define DIR_DDR DDRD
#define DIR_PORT PORTD
#define STEP_DDR DDRF
#define STEP_PORT PORTF
#define mSetDir(p, b) ((p) = ((p) & 0xf0) | ((b) & 0xf))   // write low nibble 3-0
#define mSetStep(p, b) ((p) = ((p) & 0xf) | ((b) & 0xf0))  // write high nibble 7-4

/* Macro for checking "all banks free" on UESTA0X. */
#define mBankClear(r) ((r & 0x3) == 0)

#endif //_USBCFG_H
