/************************************************************************************\

  usb.h - ATmega32U4 usb to parallel port step/direction driver board

  (c) 2008-2020 Copyright Eckler Software

  Author: David Suffield, dsuffiel@ecklersoft.com

  History:

  See makefile.
  
\************************************************************************************/
#ifndef USB_H
#define USB_H

#include <stdint.h>
#include <avr/io.h>                    // Use AVR-GCC library
#include <avr/pgmspace.h>
#include <avr/interrupt.h>
#include <avr/eeprom.h>
#include <avr/boot.h>
#define F_CPU 16000000UL 
#include <util/delay.h>

#include "usbcfg.h"
#include "usbdsc.h"
#include "usbdrv.h"
#include "usbctrltrf.h"
#include "usb9.h"

extern uint8_t bug;
extern uint8_t EEMEM ebug_byte;
extern uint16_t EEMEM ebug_word;
extern uint8_t EEMEM ebug_buf[8];

void rt_check_request(void);
void rt_init_ep(void);
void bug_blinky(uint16_t line_num, uint8_t cnt);
uint8_t hash8(uint8_t *key);

#endif //USB_H
