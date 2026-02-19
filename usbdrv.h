/************************************************************************************\

  usbdrv.h - ATmega32U4 USB driver

  (c) 2008-2020 Copyright Eckler Software

  Author: David Suffield, dsuffiel@ecklersoft.com

  History:

  See makefile.
  
\************************************************************************************/

#ifndef _USBDRV_H
#define _USBDRV_H

//#define Sleep()  asm("sleep")

/* USB Device States - To be used with [byte usb_device_state] */
#define DETACHED_STATE          0
#define ATTACHED_STATE          1
#define POWERED_STATE           2
#define DEFAULT_STATE           3
#define ADR_PENDING_STATE       4
#define ADDRESS_STATE           5
#define CONFIGURED_STATE        6

/* Memory Types for Control Transfer - used in USB_DEVICE_STATUS */
#define _RAM 0
#define _ROM 1

/* Control Transfer States */
#define WAIT_SETUP          0
#define CTRL_TRF_NOP        0
#define CTRL_TRF_TX         1
#define CTRL_TRF_RX         2

/* USB PID: Token Types - See chapter 8 in the USB specification */
#define SETUP_TOKEN         0xd
#define OUT_TOKEN           0x1
#define IN_TOKEN            0x9 

/* bmRequestType Definitions */
#define HOST_TO_DEV         (0<<7)
#define DEV_TO_HOST         (1<<7)

#define STANDARD            (0<<5)
#define CLASS               (1<<5)
#define VENDOR              (2<<5)

#define RCPT_DEV            (0)
#define RCPT_INTF           (1)
#define RCPT_EP             (2)
#define RCPT_OTH            (3)

/* bmRequestType masks */
#define mRequestDir(r) (r & 0x80)  /* transfer direction */
#define mRequestType(r) (r & 0x60) /* type */
#define mRequestRecp(r) (r & 0x1f) /* recipient */

/* UEPn Initialization Parameters */
#define EP_CTRL     _EPINEN|_EPOUTEN  // Cfg Control pipe for this ep
#define EP_OUT      _EPCONDIS|_EPOUTEN // Cfg OUT only pipe for this ep
#define EP_IN       _EPCONDIS|_EPINEN  // Cfg IN only pipe for this ep
#define EP_OUT_IN   _EPCONDIS|_EPOUTEN|_EPINEN  // Cfg both OUT & IN pipes for this ep
#define HSHK_EN     _EPHSHK        // Enable handshake packet
                                    // Handshake should be disable for isoch

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
                                   
#define EP0_CTRL 0

extern byte usb_device_state;
extern byte usb_active_cfg;
extern byte usb_alt_intf[MAX_NUM_INT];
extern byte sn_check_ok;

void USBCheckBusStatus(void);
void USBDriverService(void);
void USBPrepareForNextSetupTrf(void);
void USBDriverInitialize(void);

#endif //_USBDRV_H
