/************************************************************************************\

  usbctrltrf.h - ATmega32U4 configuration

  (c) 2008-2020 Copyright Eckler Software

  Author: David Suffield, dsuffiel@ecklersoft.com

  History:

  See makefile.
  
\************************************************************************************/

#ifndef USBCTRLTRF_H
#define USBCTRLTRF_H

#include "typedefs.h"

extern byte ctrl_trf_session_owner;
extern byte ctrl_trf_mem;               // source location _RAM | _ROM
extern PGM_VOID_P pSrc;
extern void *pDst;
extern word wCount;

/* Setup packet data */
extern byte bmRequestType;
extern byte bRequest;
extern word wValue;
extern word wIndex;
extern word wLength;

extern byte CtrlTrfData[EP0_BUFF_SIZE];

void USBCtrlEPService(void);
void USBCtrlTrfTxService(void);
void USBCtrlTrfRxService(void);
void USBCtrlEPServiceComplete(void);
void USBPrepareForNextSetupTrf(void);
void USBCtrlTrfSetupHandler(void);
void USBCtrlTrfOutHandler(void);
void USBCtrlTrfInHandler(void);

#endif //USBCTRLTRF_H
