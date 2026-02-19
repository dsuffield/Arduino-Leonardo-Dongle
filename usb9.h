/************************************************************************************\

  usb9.h - ATmega32U4 USB driver

  (c) 2008-2020 Copyright Eckler Software

  Author: David Suffield, dsuffiel@ecklersoft.com

  History:

  See makefile.
  
\************************************************************************************/

#ifndef USB9_H
#define USB9_H

#include "typedefs.h"

/******************************************************************************
 * Standard Request Codes
 * USB 2.0 Spec Ref Table 9-4
 *****************************************************************************/
#define GET_STATUS  0
#define CLR_FEATURE 1
#define SET_FEATURE 3
#define SET_ADR     5
#define GET_DSC     6
#define SET_DSC     7
#define GET_CFG     8
#define SET_CFG     9
#define GET_INTF    10
#define SET_INTF    11
#define SYNCH_FRAME 12

/* Standard Feature Selectors */
#define DEVICE_REMOTE_WAKEUP    0x01
#define ENDPOINT_HALT           0x00

void USBCheckStdRequest(void);
void USBCheckVendorRequest(void);

#endif //USB9_H
