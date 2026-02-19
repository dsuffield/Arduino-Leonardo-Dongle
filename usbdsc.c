/************************************************************************************\

  usbdsc.c - ATmega32U4 USB descriptors

  (c) 2008-2020 Copyright Eckler Software

  Author: David Suffield, dsuffiel@ecklersoft.com
 
  THIS SOFTWARE IS PROVIDED IN AN 'AS IS' CONDITION. NO WARRANTIES,
  WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING, BUT NOT LIMITED
  TO, IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
  PARTICULAR PURPOSE APPLY TO THIS SOFTWARE. THE COMPANY SHALL NOT,
  IN ANY CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL OR
  CONSEQUENTIAL DAMAGES, FOR ANY REASON WHATSOEVER.

\************************************************************************************/

/*********************************************************************
 * This file contains the USB descriptor information. It is used
 * in conjunction with the usbdsc.h file. When a descriptor is added
 * or removed from the main configuration descriptor, i.e. CFG01,
 * the user must also change the descriptor structure defined in
 * the usbdsc.h file. The structure is used to calculate the 
 * descriptor size, i.e. sizeof(CFG01).
 * 
 * A typical configuration descriptor consists of:
 * At least one configuration descriptor (USB_CFG_DSC)
 * One or more interface descriptors (USB_INTF_DSC)
 * One or more endpoint descriptors (USB_EP_DSC)
\************************************************************************************/
 
#include "typedefs.h"
#include "usb.h"

/* Device Descriptor */
const USB_DEV_DSC device_dsc PROGMEM =
{    
    sizeof(USB_DEV_DSC),    // Size of this descriptor in bytes
    DSC_DEV,                // DEVICE descriptor type
    0x0200,                 // USB Spec Release Number in BCD format
    0x00,                   // Class Code (defined at interface level)
    0x00,                   // Subclass code
    0x00,                   // Protocol code
    EP0_BUFF_SIZE,          // Max packet size for EP0, see usbcfg.h
    0x04D8,                 // Vendor ID
    0xff45,                 // Product ID: (sublicense from Microchip, signed January 7, 2009) 
    0x0000,                 // Device release number in BCD format
    0x01,                   // Manufacturer string index
    0x02,                   // Product string index
    0x03,                   // Device serial number string index
    0x01                    // Number of possible configurations
};

/* Configuration 1 Descriptor */
const struct config1 cfg01 PROGMEM =
{
    /* Configuration Descriptor */
  {
    sizeof(USB_CFG_DSC),    // Size of this descriptor in bytes
    DSC_CFG,                // CONFIGURATION descriptor type
    sizeof(cfg01),          // Total length of data for this cfg
    1,                      // Number of interfaces in this cfg
    1,                      // Index value of this configuration
    0,                      // Configuration string index
    _DEFAULT,               // Attributes, see usbdefs_std_dsc.h
    50,                     // Max power consumption (2X mA)
  },
    
    /* Interface Descriptor */
  {
    sizeof(USB_INTF_DSC),   // Size of this descriptor in bytes
    DSC_INTF,               // INTERFACE descriptor type
    0,                      // Interface Number
    0,                      // Alternate Setting Number
    1,                      // Number of endpoints in this intf
    0xff,                   // Class code (vendor specfic)
    0x00,                   // Subclass code
    0x01,                   // Protocol code
    0,                      // Interface string index
  },
   
    /* Endpoint Descriptors */
    { sizeof(USB_EP_DSC),DSC_EP,_EP01_OUT,_BULK,RT_EP_SIZE,0x00 },
//    { sizeof(USB_EP_DSC),DSC_EP,_EP01_IN,_BULK,RT_EP_SIZE,0x00 }
};

const struct _sd000 sd000 PROGMEM = { sizeof(sd000),DSC_STR, {0x0409} }; /* sd000 = supported language descriptor */

const struct _sd001 sd001 PROGMEM = { sizeof(sd001),DSC_STR,
 { 'E','c','k','l','e','r',' ','S','o','f','t','w','a','r','e'} };

const struct _sd002 sd002 PROGMEM = { sizeof(sd002),DSC_STR,
 { 'r','t','-','s','t','e','p','p','e','r',' ','3', RT_VERSION_CODE} };

/* 
 * Serial number format: yymmddn
 *   yy = two digit year 
 *   mm = two digit month 
 *   dd = two digit day (current date)
 *    n = sequential number (1-n)
 */
const struct _sd003 sd003 PROGMEM = { sizeof(sd003),DSC_STR,
 { '2','2','0','5','1','1','2','3','0'} };

/* Hash tag is generated with hash_gen.py */
const byte avr_hash PROGMEM = 0x1e;

/* AVR serial number, initialized by USBDriverInitialize() in usbdrv.c */
struct _avr_sn avr_sn;

// Microsoft OS String Descriptor
const struct USB_SD_OS usb_sd_os PROGMEM = { sizeof(usb_sd_os), DSC_STR, {'M','S','F','T','1','0','0', MS_OS_VENDOR_CODE} };

// Microsoft Compat ID OS Feature Descriptor
const struct Ext_CID_OS_FD ext_cid_os_fd PROGMEM = { sizeof(ext_cid_os_fd), 0x0100, 0x0004, 0x01, {0},
                    0x00, 0x01, {'W','I','N','U','S','B',0}, {0}, {0} };
                 // ^^^^ This must match your interface number

// Microsoft Extended Properties Feature Descriptor
const struct Ext_P_OS_FD ext_p_os_fd PROGMEM = {sizeof(ext_p_os_fd), 0x0100, 0x0005, 0x0001,
                0x00000084, 0x00000001, 
                0x0028,     {'D','e','v','i','c','e','I','n','t','e','r','f','a','c','e','G','U','I','D',0},
                0x0000004E, {'{','7','A','7','1','2','B','2','0','-','A','6','6','C','-','4','c','6','f','-',
                             'A','C','2','C','-','9','A','D','D','9','1','3','C','1','7','C','2','}',0}};
