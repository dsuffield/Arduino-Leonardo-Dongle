/************************************************************************************\

  usbdrv.c - ATmega32U4 USB driver

  (c) 2008-2020 Copyright Eckler Software

  Author: David Suffield, dsuffiel@ecklersoft.com
 
  THIS SOFTWARE IS PROVIDED IN AN 'AS IS' CONDITION. NO WARRANTIES,
  WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING, BUT NOT LIMITED
  TO, IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
  PARTICULAR PURPOSE APPLY TO THIS SOFTWARE. THE COMPANY SHALL NOT,
  IN ANY CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL OR
  CONSEQUENTIAL DAMAGES, FOR ANY REASON WHATSOEVER.

\************************************************************************************/

#include "usb.h"

void USBModuleEnable(void);
void USBModuleDisable(void);
void USBSuspend(void);
void USBWakeFromSuspend(void);
void USBProtocolResetHandler(void);
void USB_SOF_Handler(void);
void USBStallHandler(void);
void USBErrorHandler(void);

byte usb_device_state;          // Device States: DETACHED, ATTACHED, ...
byte usb_active_cfg;            // Value of current configuration
byte usb_alt_intf[MAX_NUM_INT]; // Array to keep track of the current alternate
                                // setting for each interface ID

#define VBUS_DEBOUNCE_MAX 7936      // ~0.5ms
static uint16_t vbus_hi_debounce;
static uint16_t vbus_lo_debounce;

/* ATmega32U4 serial number example: 57-38-39-32-31-34-15-17-0f-0f */
uint8_t serial_num[11];   /* binary */
byte sn_check_ok;

char nibbleToHex(uint8_t n)
{
  if (n <= 9) { return '0' + n; }
  else { return 'a' + (n - 10); }
}

/******************************************************************************
 * Function:        void USBDriverInitialize(void)
 *
 * Overview:        Configures the USB driver module
 * 
 *****************************************************************************/
void USBDriverInitialize(void)
{
    byte *pb;
    word *pw;

    /* Mask all USB interrupts */
    UDIEN = 0;
    UEIENX = 0;
    USBCON &= ~_BV(VBUSTE);

    /* Read AVR serial number */
    pb = serial_num;
    *pb++ = boot_signature_byte_get(0x0e);
    *pb++ = boot_signature_byte_get(0x0f);
    *pb++ = boot_signature_byte_get(0x10);
    *pb++ = boot_signature_byte_get(0x11);
    *pb++ = boot_signature_byte_get(0x12);
    *pb++ = boot_signature_byte_get(0x13);
    *pb++ = boot_signature_byte_get(0x14);
    *pb++ = boot_signature_byte_get(0x15);
    *pb++ = boot_signature_byte_get(0x16);
    *pb++ = boot_signature_byte_get(0x17);
    *pb = 0;   /* null terminate */

    /* Init USB string descriptor with AVR serial number */
    avr_sn.bLength = sizeof(avr_sn);
    avr_sn.bDscType = DSC_STR;
    for (pb=serial_num, pw=avr_sn.string; *pb; pb++)
    {
        *pw++ = nibbleToHex(*pb >> 4);
        *pw++ = nibbleToHex(*pb & 0xf);
    }

    /* Verify FW for this AVR */
//    sn_check_ok = (hash8(serial_num) == pgm_read_byte_near(&avr_hash)) ? TRUE : FALSE;
    sn_check_ok = TRUE;  /* disable check, make this version a freebie 5-11-2022 DES */

    /* Enable USB pad regulator */
    UHWCON |= _BV(UVREGE);

    /* Enable VBUS pad */
    USBCON |= _BV(OTGPADE);

    usb_device_state = DETACHED_STATE;
}// end USBDriverInitialize

/******************************************************************************
 * Function:        void USBCheckBusStatus(void)
 *
 * PreCondition:    None
 *
 * Input:           None
 *
 * Output:          None
 *
 * Side Effects:    None
 *
 * Overview:        This routine enables/disables the USB module by monitoring
 *                  the USB power signal.
 *
 * Note:            None
 *****************************************************************************/
void USBCheckBusStatus(void)
{
    if (bit_is_set(USBINT, VBUSTI))       // Is USB VBUS transition?
    {
        if (bit_is_set(USBSTA, VBUS))       // Is VBUS high?
        {
            if (++vbus_hi_debounce >= VBUS_DEBOUNCE_MAX)
            {
                /*
                 * Make sure we are detached before calling USBModuleEnable().
                 * This works around the Sherline CNC controller 5v brown-out or power surge
                 * issue when turning on the controller. This forces the dongle to re-enumerate.
                 * The 5v surge causes a VBUS transtion because the 5v drops to < 1.4v.  6/3/2020 DES
                 */
                USBModuleDisable();  // make sure we detached

                USBModuleEnable();     // Is on, enable it
                vbus_hi_debounce = 0;
                USBINT &= ~_BV(VBUSTI);  // Reset the VBUSTI interrupt flag
            }
            vbus_lo_debounce = 0;
        }
        else if (bit_is_clear(USBSTA, VBUS))  // Is VBUS low?
        {
            if (++vbus_lo_debounce >= VBUS_DEBOUNCE_MAX)
            {
                USBModuleDisable();    // Is off, disable it
                vbus_lo_debounce = 0;
                USBINT &= ~_BV(VBUSTI);  // Reset the VBUSTI interrupt flag
            }
            vbus_hi_debounce = 0;
        }
    }
}//end USBCheckBusStatus

/******************************************************************************
 * Function:        void USBModuleEnable(void)
 *
 * PreCondition:    None
 *
 * Input:           None
 *
 * Output:          None
 *
 * Side Effects:    None
 *
 * Overview:        This routine enables the USB module.
 *                  An end designer should never have to call this routine
 *                  manually. This routine should only be called from
 *                  USBCheckBusStatus().
 *
 * Note:            See USBCheckBusStatus() for more information.
 *****************************************************************************/
void USBModuleEnable(void)
{
    /* Mask all USB interrupts */
    UDIEN = 0;
    UEIENX = 0;
    USBCON &= ~_BV(VBUSTE);

    /* Disable USB controller */
    USBCON &= ~_BV(USBE);

    /* Freeze clock */
    USBCON |= _BV(FRZCLK);

    /* Start 96mhz PLL */
    PLLFRQ = _BV(PLLUSB) | _BV(PDIV3) | _BV(PDIV1);  // High-Speed Timer = disconnected
    PLLCSR = _BV(PINDIV) | _BV(PLLE);

    /* Wait for PLL lock, takes several ms */
    while (bit_is_clear(PLLCSR, PLOCK));

    /* Unfreeze clock */
    USBCON &= ~_BV(FRZCLK);

    /* Enable USB controller */
    USBCON |= _BV(USBE);

    /* Attach */
    UDCON  &= ~_BV(DETACH);

    /* Disable CPU reset when USB End-Of-Reset occurs. */
    UDCON &= ~_BV(RSTCPU);

    usb_device_state = ATTACHED_STATE;      // Defined in usbdrv.c & .h
}//end USBModuleEnable

/******************************************************************************
 * Function:        void USBModuleDisable(void)
 *
 * PreCondition:    None
 *
 * Input:           None
 *
 * Output:          None
 *
 * Side Effects:    None
 *
 * Overview:        This routine disables the USB module.
 *                  An end designer should never have to call this routine
 *                  manually. This routine should only be called from
 *                  USBCheckBusStatus().
 *
 * Note:            See USBCheckBusStatus() for more information.
 *****************************************************************************/
void USBModuleDisable(void)
{
    /* Mask all USB interrupts */
    UDIEN = 0;
    UEIENX = 0;
    USBCON &= ~_BV(VBUSTE);

    /* Detach */
    UDCON  |= _BV(DETACH);

    /* Disable USB controller */
    USBCON &= ~_BV(USBE);

    /* Freeze clock */
    USBCON |= _BV(FRZCLK);

    /* Stop PLL */
    PLLCSR |= _BV(PLLE);

    usb_device_state = DETACHED_STATE;      // Defined in usbdrv.c & .h
}//end USBModuleDisable

/******************************************************************************
 * Function:        void USBDriverService(void)
 *
 * PreCondition:    None
 *
 * Input:           None
 *
 * Output:          None
 *
 * Side Effects:    None
 *
 * Overview:        This routine is the heart of this firmware. It manages
 *                  all USB interrupts.
 *
 * Note:            Device state transitions through the following stages:
 *                  DETACHED -> ATTACHED -> POWERED -> DEFAULT ->
 *                  ADDRESS_PENDING -> ADDRESSED -> CONFIGURED -> READY
 *****************************************************************************/
void USBDriverService(void)
{   
    /*
     * Pointless to continue servicing if USB cable is not even attached.
     */
    if(usb_device_state == DETACHED_STATE)
       return;
    
    /*
     * Task A: Service USB Activity Interrupt
     */
    if(bit_is_set(UDINT, WAKEUPI))
    {
       USBWakeFromSuspend();
    }

    /*
     * Pointless to continue servicing if the device is in suspend mode.
     */    
    if(bit_is_set(UDINT, SUSPI))
    {
        USBSuspend();        /* 3ms idle condition put CPU into suspend mode */
        return;
    }
 
    /*
     * Task B: Service USB Bus Reset Interrupt.
     * When bus reset is received during suspend, ACTVIF will be set first,
     * once the UCONbits.SUSPND is clear, then the URSTIF bit will be asserted.
     * This is why URSTIF is checked after ACTVIF.
     */
    if(bit_is_set(UDINT, EORSTI))
    {
       USBProtocolResetHandler();
    }
    
    /*
     * Task C: Service other USB interrupts
     */
    if(bit_is_set(UDINT, SOFI))
    {
        USB_SOF_Handler();
    }

    UENUM = EP0_CTRL;  /* select EP */

    if(bit_is_set(UEINTX, STALLEDI))
    {
        USBStallHandler();
    }
    if(bit_is_set(UESTA0X, OVERFI) || bit_is_set(UESTA0X, UNDERFI))
    {
        USBErrorHandler();
    }

    /*
     * Pointless to continue servicing if the host has not sent a bus reset.
     * Once bus reset is received, the device transitions into the DEFAULT
     * state and is ready for communication.
     */
    if(usb_device_state < DEFAULT_STATE)
    {
       return;
    }

    /*
     * Task D: Servicing USB Transaction Complete Interrupt
     */
    
    /* 
     * Check for EP0 SETUP, OUT or IN transaction. Only service transactions over EP0.
     * Ignore all other EP transactions.
     *
     * Other EP can be serviced later by responsible device class firmware.
     * Each device driver knows when an OUT or IN transaction is ready by
     * selecting their EP.
     * An OUT EP should always be owned by SIE until the data is ready.
     * An IN EP should always be owned by CPU until the data is ready.
     */
    if(bit_is_set(UEINTX, RXSTPI))
    {
        USBCtrlTrfSetupHandler();
    }
    else if (bit_is_set(UEINTX, RXOUTI))
    {
        USBCtrlTrfOutHandler();
    }
    else if (bit_is_set(UEINTX, TXINI))
    {
        USBCtrlTrfInHandler();
    }
    
}//end USBDriverService

/******************************************************************************
 * Function:        void USBSuspend(void)
 *
 * PreCondition:    None
 *
 * Input:           None
 *
 * Output:          None
 *
 * Side Effects:    None
 *
 * Overview:        
 *
 * Note:            None
 *****************************************************************************/
void USBSuspend(void)
{
    /*
     * At this point the PIC can go into sleep,idle, or
     * switch to a slower clock, etc.
     */
    
    if (bit_is_set(USBCON, FRZCLK))
        return;

    /* Freeze clock */
    USBCON |= _BV(FRZCLK);

    /* Leave SUSPI set, WAKEUPI will clear SUSPI */

    /* Modifiable Section */
// Disabled going to sleep, DES 10/25/2016
//    PIR2bits.USBIF = 0;
//    PIE2bits.USBIE = 1;                     // Set USB wakeup source
//    Sleep();                                // Goto sleep
//    PIE2bits.USBIE = 0;
    /* End Modifiable Section */

}//end USBSuspend

/******************************************************************************
 * Function:        void USBWakeFromSuspend(void)
 *
 * PreCondition:    None
 *
 * Input:           None
 *
 * Output:          None
 *
 * Side Effects:    None
 *
 * Overview:        
 *
 * Note:            None
 *****************************************************************************/
void USBWakeFromSuspend(void)
{
    /* 
     * If using clock switching, this is the place to restore the
     * original clock frequency.
     */

    /* Unfreeze clock */
    USBCON &= ~_BV(FRZCLK);

    /* Clear the Wakeup Interrupt, hardware clears the Suspend Interrupt */
    UDINT &= ~_BV(WAKEUPI);
}//end USBWakeFromSuspend

/******************************************************************************
 * Function:        void USBRemoteWakeup(void)
 *
 * PreCondition:    None
 *
 * Input:           None
 *
 * Output:          None
 *
 * Side Effects:    None
 *
 * Overview:        This function should be called by user when the device
 *                  is waken up by an external stimulus other than ACTIVIF.
 *                  Please read the note below to understand the limitations.
 *
 * Note:            The modifiable section in this routine should be changed
 *                  to meet the application needs. Current implementation
 *                  temporary blocks other functions from executing for a
 *                  period of 1-13 ms depending on the core frequency.
 *
 *                  According to USB 2.0 specification section 7.1.7.7,
 *                  "The remote wakeup device must hold the resume signaling
 *                  for at lest 1 ms but for no more than 15 ms."
 *                  The idea here is to use a delay counter loop, using a
 *                  common value that would work over a wide range of core
 *                  frequencies.
 *                  That value selected is 1800. See table below:
 *                  ==========================================================
 *                  Core Freq(MHz)      MIP         RESUME Signal Period (ms)
 *                  ==========================================================
 *                      48              12          1.05
 *                       4              1           12.6
 *                  ==========================================================
 *                  * These timing could be incorrect when using code
 *                    optimization or extended instruction mode,
 *                    or when having other interrupts enabled.
 *                    Make sure to verify using the MPLAB SIM's Stopwatch
 *****************************************************************************/
void USBRemoteWakeup(void)
{
#if 0
    static word delay_count;
    
    if(usb_stat.RemoteWakeup == 1)          // Check if RemoteWakeup function
    {                                       // has been enabled by the host.
        USBWakeFromSuspend();               // Unsuspend USB modue
        UCONbits.RESUME = 1;                // Start RESUME signaling

        /* Modifiable Section */
        
        delay_count = 1800U;                // Set RESUME line for 1-13 ms
        do
        {
            delay_count--;
        }while(delay_count);        
        
        /* End Modifiable Section */
        
        UCONbits.RESUME = 0;
    }//endif 
#endif
}//end USBRemoteWakeup

/******************************************************************************
 * Function:        void USB_SOF_Handler(void)
 *
 * PreCondition:    None
 *
 * Input:           None
 *
 * Output:          None
 *
 * Side Effects:    None
 *
 * Overview:        The USB host sends out a SOF packet to full-speed devices
 *                  every 1 ms. This interrupt may be useful for isochronous
 *                  pipes. End designers should implement callback routine
 *                  as necessary.
 *
 * Note:            None
 *****************************************************************************/
void USB_SOF_Handler(void)
{
    /* Callback routine here */

    /* Clear the SOF interrupt flag */
    UDINT &= ~_BV(SOFI);
}//end USB_SOF_Handler

/******************************************************************************
 * Function:        void USBStallHandler(void)
 *
 * PreCondition:    UENUM = EP, A STALL packet is sent to the host by the SIE.
 *
 * Input:           
 *
 * Output:          None
 *
 * Side Effects:    None
 *
 * Overview:        The STALLIF is set anytime the SIE sends out a STALL
 *                  packet regardless of which endpoint causes it.
 *                  A Setup transaction overrides the STALL function. A stalled
 *                  endpoint stops stalling once it receives a setup packet.
 *                  In this case, the SIE will accepts the Setup packet and
 *                  set the TRNIF flag to notify the firmware. STALL function
 *                  for that particular endpoint pipe will be automatically
 *                  disabled (direction specific).
 *
 *                  There are a few reasons for an endpoint to be stalled.
 *                  1. When a non-supported USB request is received.
 *                     Example: GET_DESCRIPTOR(DEVICE_QUALIFIER)
 *                  2. When an endpoint is currently halted.
 *                  3. When the device class specifies that an endpoint must
 *                     stall in response to a specific event.
 *                     Example: Mass Storage Device Class
 *                              If the CBW is not valid, the device shall
 *                              STALL the Bulk-In pipe.
 *                              See USB Mass Storage Class Bulk-only Transport
 *                              Specification for more details.
 *
 * Note:            UEPn.EPSTALL can be scanned to see which endpoint causes
 *                  the stall event.
 *                  If
 *****************************************************************************/
void USBStallHandler(void)
{
    /*
     * Does not really have to do anything here,
     * even for the control endpoint.
     * All BDs of Endpoint 0 are owned by SIE right now,
     * but once a Setup Transaction is received, the ownership
     * for EP0_OUT will be returned to CPU.
     * When the Setup Transaction is serviced, the ownership
     * for EP0_IN will then be forced back to CPU by firmware.
     */

    //UECONX |= _BV(STALLRQC);         /* reset stall request */
    UEINTX &= ~_BV(STALLEDI);         /* reset stall interrupt flag */
}//end USBStallHandler

/******************************************************************************
 * Function:        void USBErrorHandler(void)
 *
 * PreCondition:    UENUM = EP
 *
 * Input:           None
 *
 * Output:          None
 *
 * Side Effects:    None
 *
 * Overview:        The purpose of this interrupt is mainly for debugging
 *                  during development. Check UEIR to see which error causes
 *                  the interrupt.
 *
 * Note:            None
 *****************************************************************************/
void USBErrorHandler(void)
{
    UESTA0X &= ~_BV(OVERFI);   /* reset overflow error */
    UESTA0X &= ~_BV(UNDERFI);  /* reset underflow error */
}//end USBErrorHandler

/******************************************************************************
 * Function:        void USBProtocolResetHandler(void)
 *
 * PreCondition:    A USB bus reset is received from the host.
 *
 * Input:           None
 *
 * Output:          None
 *
 * Side Effects:    Currently, this routine flushes any pending USB
 *                  transactions. It empties out the USTAT FIFO. This action
 *                  might not be desirable in some applications.
 *
 * Overview:        Once a USB bus reset is received from the host, this
 *                  routine should be called. It resets the device address to
 *                  zero, disables all non-EP0 endpoints, initializes EP0 to
 *                  be ready for default communication, clears all USB
 *                  interrupt flags, unmasks applicable USB interrupts, and
 *                  reinitializes internal state-machine variables.
 *
 * Note:            None
 *****************************************************************************/
void USBProtocolResetHandler(void)
{
    /* Reset state machine, FIFO, UEINTX, UESTA0X, UESTA1X for EP0-EP6*/
    UERST |= _BV(EPRST6) | _BV(EPRST5) | _BV(EPRST4) | _BV(EPRST3) | _BV(EPRST2) | _BV(EPRST1) | _BV(EPRST0);

    /* Clear EP0-EP6 reset */
    UERST &= ~(_BV(EPRST6) | _BV(EPRST5) | _BV(EPRST4) | _BV(EPRST3) | _BV(EPRST2) | _BV(EPRST1) | _BV(EPRST0));

    /* Reset to default address */
    UDADDR = 0;

    /* Clear all USB interrupts */
    UDINT = 0;
    
    USBPrepareForNextSetupTrf();    // Declared in usbctrltrf.c

    usb_active_cfg = 0;             // Clear active configuration
    usb_device_state = DEFAULT_STATE;

    /* Initialize control EP0 */
    UENUM = EP0_CTRL;  /* select EP */
    UECONX |= _BV(EPEN); /* activate EP */
    UECFG0X = 0;         /* EPTYPE=Control, EPDIR=OUT */
    UECFG1X = 0; /* EPSIZE=8 bytes, EPBK=One bank */
    UECFG1X |= _BV(ALLOC); /* allocate data buffer */

    //if (bit_is_clear(UESTA0X, CFGOK))
    //    bug_blinky(__LINE__);  /* EP config error */
    
}//end USBProtocolResetHandler
