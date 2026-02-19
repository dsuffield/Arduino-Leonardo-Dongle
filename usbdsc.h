/************************************************************************************\

  usbdsc.h - ATmega32U4 USB descriptors

  (c) 2008-2020 Copyright Eckler Software

  Author: David Suffield, dsuffiel@ecklersoft.com

  History:

  See makefile.
  
\************************************************************************************/

#ifndef USBDSC_H
#define USBDSC_H

#include "typedefs.h"

/* Descriptor Types */
#define DSC_DEV     0x01
#define DSC_CFG     0x02
#define DSC_STR     0x03
#define DSC_INTF    0x04
#define DSC_EP      0x05

/* String descriptor index */
#define LANG_INDEX            0x00
#define MAN_INDEX             0x01
#define PROD_INDEX            0x02   
#define SN_INDEX              0x03
#define MICROSOFT_OS_INDEX    0xee

/******************************************************************************
 * USB Endpoint Definitions
 * USB Standard EP Address Format: DIR:X:X:X:EP3:EP2:EP1:EP0
 * This is used in the descriptors. See autofiles\usbdsc.c
 * 
 * NOTE: Do not use these values for checking against USTAT.
 * To check against USTAT, use values defined in "system\usb\usbdrv\usbdrv.h"
 *****************************************************************************/
#define _EP01_OUT   0x01
#define _EP01_IN    0x81
#define _EP02_OUT   0x02
#define _EP02_IN    0x82
#define _EP03_OUT   0x03
#define _EP03_IN    0x83
#define _EP04_OUT   0x04
#define _EP04_IN    0x84
#define _EP05_OUT   0x05
#define _EP05_IN    0x85
#define _EP06_OUT   0x06
#define _EP06_IN    0x86
#define _EP07_OUT   0x07
#define _EP07_IN    0x87
#define _EP08_OUT   0x08
#define _EP08_IN    0x88
#define _EP09_OUT   0x09
#define _EP09_IN    0x89
#define _EP10_OUT   0x0A
#define _EP10_IN    0x8A
#define _EP11_OUT   0x0B
#define _EP11_IN    0x8B
#define _EP12_OUT   0x0C
#define _EP12_IN    0x8C
#define _EP13_OUT   0x0D
#define _EP13_IN    0x8D
#define _EP14_OUT   0x0E
#define _EP14_IN    0x8E
#define _EP15_OUT   0x0F
#define _EP15_IN    0x8F

/* Configuration Attributes */
#define _DEFAULT    0x01<<7         //Default Value (Bit 7 is set)
#define _SELF       0x01<<6         //Self-powered (Supports if set)
#define _RWU        0x01<<5         //Remote Wakeup (Supports if set)

/* Endpoint Transfer Type */
#define _CTRL       0x00            //Control Transfer
#define _ISO        0x01            //Isochronous Transfer
#define _BULK       0x02            //Bulk Transfer
#define _INT        0x03            //Interrupt Transfer

/* Isochronous Endpoint Synchronization Type */
#define _NS         0x00<<2         //No Synchronization
#define _AS         0x01<<2         //Asynchronous
#define _AD         0x02<<2         //Adaptive
#define _SY         0x03<<2         //Synchronous

/* Isochronous Endpoint Usage Type */
#define _DE         0x00<<4         //Data endpoint
#define _FE         0x01<<4         //Feedback endpoint
#define _IE         0x02<<4         //Implicit feedback Data endpoint

/******************************************************************************
 * USB Device Descriptor Structure
 *****************************************************************************/
typedef struct _USB_DEV_DSC
{
    byte bLength;       byte bDscType;      word bcdUSB;
    byte bDevCls;       byte bDevSubCls;    byte bDevProtocol;
    byte bMaxPktSize0;  word idVendor;      word idProduct;
    word bcdDevice;     byte iMFR;          byte iProduct;
    byte iSerialNum;    byte bNumCfg;
} USB_DEV_DSC;

/******************************************************************************
 * USB Configuration Descriptor Structure
 *****************************************************************************/
typedef struct _USB_CFG_DSC
{
    byte bLength;       byte bDscType;      word wTotalLength;
    byte bNumIntf;      byte bCfgValue;     byte iCfg;
    byte bmAttributes;  byte bMaxPower;
} USB_CFG_DSC;

/******************************************************************************
 * USB Interface Descriptor Structure
 *****************************************************************************/
typedef struct _USB_INTF_DSC
{
    byte bLength;       byte bDscType;      byte bIntfNum;
    byte bAltSetting;   byte bNumEPs;       byte bIntfCls;
    byte bIntfSubCls;   byte bIntfProtocol; byte iIntf;
} USB_INTF_DSC;

/******************************************************************************
 * USB Endpoint Descriptor Structure
 *****************************************************************************/
typedef struct _USB_EP_DSC
{
    byte bLength;       byte bDscType;      byte bEPAdr;
    byte bmAttributes;  word wMaxPktSize;   byte bInterval;
} USB_EP_DSC;

struct config1                            
{   USB_CFG_DSC             cd01;                   
    USB_INTF_DSC            i00a00;                 
    USB_EP_DSC              ep01i_i00a00;           
//    USB_EP_DSC              ep01o_i00a00;           
};

struct USB_SD_OS
{
   byte bLength;
   byte bDscType; 
   word string[8];
};

struct _sd000 
{
   byte bLength;byte bDscType;word string[1];
};

struct _sd001 
{
   byte bLength;byte bDscType;word string[16];
};

struct _sd002 
{
   byte bLength;byte bDscType;word string[16];
};

struct _sd003 
{
	byte bLength;byte bDscType;word string[16];
};

struct _avr_sn
{
	byte bLength;byte bDscType;word string[21];
};

struct Ext_CID_OS_FD
{
	// Header
	dword dwlength;
	word  bcdVersion;
	word  wIndex;
	byte  bCount;
	byte  bReserved1[7];
	// Function Section 1
	byte  bFirstInterfaceNumber;
	byte  bReserved2;
	char  bCompatibleID[8];
	byte  bSubCompatibleID[8];
	byte  bReserved3[6];
};

struct Ext_P_OS_FD
{
	// Header
	dword dwLength;
	word  bcdVersion;
	word  wIndex;
	word  wCount;
	// Custom Property Section 1
	dword dwSize;
	dword dwPropertyDataType;
	word  wPropertyNameLength;
	word  bPropertyName[20];
	dword dwPropertyDataLength;
	word  bPropertyData[39];
};

extern const struct config1 cfg01;
extern const USB_DEV_DSC device_dsc;
extern const struct USB_SD_OS usb_sd_os;
extern const struct Ext_CID_OS_FD ext_cid_os_fd;
extern const struct Ext_P_OS_FD ext_p_os_fd;
extern const struct _sd000 sd000;
extern const struct _sd001 sd001;
extern const struct _sd002 sd002;
extern const struct _sd003 sd003;
extern struct _avr_sn avr_sn;
extern const byte avr_hash;

#endif //USBDSC_H
