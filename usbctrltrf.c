/************************************************************************************\

  usbctrltrf.c - ATmega32U4 USB driver

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

static byte ctrl_trf_state;                // Control Transfer State
byte ctrl_trf_session_owner;        // Current transfer session owner
byte ctrl_trf_mem;               // source location _RAM | _ROM

PGM_VOID_P pSrc;                       // Data source pointer
void *pDst;                       // Data destination pointer
word wCount;                        // Data count in bytes
word wTotal;                   // Data count over multiple transfers  

/* Setup packet data */
byte bmRequestType;
byte bRequest;
word wValue;
word wIndex;
word wLength;

byte bug;

/******************************************************************************
 * CtrlTrfData:
 *
 * Buffer size has to equal the EP0_BUFF_SIZE value specified
 * in usbcfg.h
 * The value of EP0_BUFF_SIZE can be 8, 16, 32, or 64.
 *
 * First 8 bytes are defined to be directly addressable to improve speed
 * and reduce code size.
 * Bytes beyond the 8th byte have to be accessed using indirect addressing.
 *****************************************************************************/
byte CtrlTrfData[EP0_BUFF_SIZE];

/******************************************************************************
 * Function:        void USBCtrlTrfSetupHandler(void)
 *
 * PreCondition:    UENUM = EP0 and buffer is loaded with valid USB Setup Data
 *
 * Input:           None
 *
 * Output:          None
 *
 * Side Effects:    None
 *
 * Overview:        This routine is a task dispatcher and has 3 stages.
 *                  1. It initializes the control transfer state machine.
 *                  2. It calls on each of the module that may know how to
 *                     service the Setup Request from the host.
 *                     Module Example: USB9, HID, CDC, MSD, ...
 *                     As new classes are added, ClassReqHandler table in
 *                     usbdsc.c should be updated to call all available
 *                     class handlers.
 *                  3. Once each of the modules has had a chance to check if
 *                     it is responsible for servicing the request, stage 3
 *                     then checks direction of the transfer to determine how
 *                     to prepare EP0 for the control transfer.
 *                     Refer to USBCtrlEPServiceComplete() for more details.
 *
 * Note:            Microchip USB Firmware has three different states for
 *                  the control transfer state machine:
 *                  1. WAIT_SETUP
 *                  2. CTRL_TRF_TX
 *                  3. CTRL_TRF_RX
 *                  Refer to firmware manual to find out how one state
 *                  is transitioned to another.
 *
 *                  A Control Transfer is composed of many USB transactions.
 *                  When transferring data over multiple transactions,
 *                  it is important to keep track of data source, data
 *                  destination, and data count. These three parameters are
 *                  stored in pSrc,pDst, and wCount. A flag is used to
 *                  note if the data source is from ROM or RAM.
 *
 *****************************************************************************/
void USBCtrlTrfSetupHandler(void)
{
    /* Stage 1 */
    ctrl_trf_state = WAIT_SETUP;
    ctrl_trf_session_owner = MUID_NULL;     // Set owner to NULL
    wCount = 0;
    wTotal = 0;
    
    /* Stage 2 */
    bmRequestType = UEDATX;
    bRequest = UEDATX;
    LSB(wValue) = UEDATX;   // low byte
    MSB(wValue) = UEDATX;    // high byte
    LSB(wIndex) = UEDATX;
    MSB(wIndex) = UEDATX;
    LSB(wLength) = UEDATX;
    MSB(wLength) = UEDATX;
    
    USBCheckStdRequest();               // See system\usb9\usb9.c

    if(ctrl_trf_session_owner == MUID_NULL)
        USBCheckVendorRequest();         // Check for OS specific Vendor Requests
    
    if(ctrl_trf_session_owner == MUID_NULL)
        rt_check_request();            // Check for rtstepper vendor requests
        
    /* Stage 3 */
    USBCtrlEPServiceComplete();
    
}//end USBCtrlTrfSetupHandler

/******************************************************************************
 * Function:        void USBCtrlTrfOutHandler(void)
 *
 * PreCondition:    UENUM = EP0
 *
 * Input:           None
 *
 * Output:          None
 *
 * Side Effects:    None
 *
 * Overview:        This routine handles an OUT transaction according to
 *                  which control transfer state is currently active.
 *
 * Note:            Note that if the the control transfer was from
 *                  host to device, the session owner should be notified
 *                  at the end of each OUT transaction to service the
 *                  received data.
 *
 *****************************************************************************/
void USBCtrlTrfOutHandler(void)
{
    if(ctrl_trf_state == CTRL_TRF_RX)
    {
        USBCtrlTrfRxService();
    }
    else 
    {
        /* Reset the OUT interrupt flag. */
        UEINTX &= ~_BV(RXOUTI);
    }

}//end USBCtrlTrfOutHandler

/******************************************************************************
 * Function:        void USBCtrlTrfInHandler(void)
 *
 * PreCondition:    UENUM = EP0
 *
 * Input:           None
 *
 * Output:          None
 *
 * Side Effects:    None
 *
 * Overview:        This routine handles an IN transaction according to
 *                  which control transfer state is currently active.
 *
 *
 * Note:            A Set Address Request must not change the acutal address
 *                  of the device until the completion of the control
 *                  transfer. The end of the control transfer for Set Address
 *                  Request is an IN transaction. Therefore it is necessary
 *                  to service this unique situation when the condition is
 *                  right. Macro mUSBCheckAdrPendingState is defined in
 *                  usb9.h and its function is to specifically service this
 *                  event.
 *****************************************************************************/
void USBCtrlTrfInHandler(void)
{
    if(usb_device_state == ADR_PENDING_STATE)
    {
        if(bit_is_clear(UDADDR, ADDEN))
        {
            usb_device_state=ADDRESS_STATE;
            UDADDR |= _BV(ADDEN);        // enable new address
        }
        else
        {
            usb_device_state=DEFAULT_STATE;
        }

        /* Reset the IN interrupt flag. */
        UEINTX &= ~_BV(TXINI);
    }

    if(ctrl_trf_state == CTRL_TRF_TX)
    {
        USBCtrlTrfTxService();     // copy into data bank

        /* Reset the IN interrupt flag. */
        UEINTX &= ~_BV(TXINI);
    }

}//end USBCtrlTrfInHandler

/******************************************************************************
 * Function:        void USBCtrlTrfTxService(void)
 *
 * PreCondition:    UENUM = EP0, pSrc, wCount, and ctrl_trf_mem are setup properly.
 *
 * Input:           None
 *
 * Output:          None
 *
 * Side Effects:    None
 *
 * Overview:        This routine should be called from only two places.
 *                  One from USBCtrlEPServiceComplete()?? and one from
 *                  USBCtrlTrfInHandler(). It takes care of managing a
 *                  transfer over multiple USB transactions.
 *
 * Note:            This routine works with isochronous endpoint larger than
 *                  256 bytes and is shown here as an example of how to deal
 *                  with BC9 and BC8. In reality, a control endpoint can never
 *                  be larger than 64 bytes.
 *****************************************************************************/
void USBCtrlTrfTxService(void)
{    
    word byte_to_send;
    //word tmp;
    
    /*
     * First, have to figure out how many byte of data to send.
     */
    if(wCount < EP0_BUFF_SIZE)
        byte_to_send = wCount;
    else
        byte_to_send = EP0_BUFF_SIZE;

#if 0
    MSB(tmp) = UEBCHX;   
    LSB(tmp) = UEBCLX;

    if (tmp)
    {  /* Debug code, use avrdude to read eeprom */
        eeprom_update_byte(&ebug_byte, UEINTX);  
        eeprom_update_word(&ebug_word, tmp);
        eeprom_update_byte(&ebug_buf[0], UEDATX);
        eeprom_update_byte(&ebug_buf[1], UEDATX);
        eeprom_update_byte(&ebug_buf[2], UEDATX);
        eeprom_update_byte(&ebug_buf[3], UEDATX);
        eeprom_update_byte(&ebug_buf[4], UEDATX);
        eeprom_update_byte(&ebug_buf[5], UEDATX);
        eeprom_update_byte(&ebug_buf[6], UEDATX);
        eeprom_update_byte(&ebug_buf[7], UEDATX);
        bug_blinky(__LINE__);
    }
#endif

    /*
     * Subtract the number of bytes just about to be sent from the total.
     */
    wCount -= byte_to_send;
    
    if(ctrl_trf_mem == _ROM)       // Determine type of memory source
    {
        while(byte_to_send)
        {
            UEDATX = pgm_read_byte_near(pSrc);
            pSrc++;
            byte_to_send--;
        }//end while(byte_to_send)
    }
    else // RAM
    {
        while(byte_to_send)
        {
            UEDATX = *(byte *)pSrc;
            pSrc++;
            byte_to_send--;
        }//end while(byte_to_send._word)
    }//end if(usb_stat.ctrl_trf_mem == _ROM)

    if (wCount == 0)
        USBPrepareForNextSetupTrf();
            
}//end USBCtrlTrfTxService

/******************************************************************************
 * Function:        void USBCtrlTrfRxService(void)
 *
 * PreCondition:    UENUM = EP0, pDst and wCount are setup properly.
 *                  pSrc is always &bdt_data.CtrlTrfData
 *                  usb_stat.ctrl_trf_mem is always _RAM.
 *                  wCount should be set to 0 at the start of each control
 *                  transfer.
 *
 * Input:           None
 *
 * Output:          None
 *
 * Side Effects:    None
 *
 * Overview:        *** This routine is only partially complete. Check for
 *                  new version of the firmware.
 *
 * Note:            None
 *****************************************************************************/
void USBCtrlTrfRxService(void)
{
    word byte_to_read;

    MSB(byte_to_read) = UEBCHX;
    LSB(byte_to_read) = UEBCLX;
    
    /*
     * Accumulate total number of bytes read
     */
    wTotal += byte_to_read;
    
    while(byte_to_read)
    {
        *(byte *)pDst = UEDATX;
        pDst++;
        byte_to_read--;
    }//end while(byte_to_read._word)   

    /* Reset the OUT interrupt flag. */
    UEINTX &= ~_BV(RXOUTI);

    /* Check for last transfer */
    if (wTotal >= wCount)
    {
        UEINTX &= ~_BV(TXINI);  // yes, send IN 0 bytes status stage

        USBPrepareForNextSetupTrf();
    }
    
}//end USBCtrlTrfRxService

/******************************************************************************
 * Function:        void USBCtrlEPServiceComplete(void)
 *
 * PreCondition:    UENUM = EP0
 *
 * Input:           None
 *
 * Output:          None
 *
 * Side Effects:    None
 *
 * Overview:        This routine wrap up the ramaining tasks in servicing
 *                  a Setup Request. Its main task is to set the endpoint
 *                  controls appropriately for a given situation. See code
 *                  below.
 *                  There are three main scenarios:
 *                  a) There was no handler for the Request, in this case
 *                     a STALL should be sent out.
 *                  b) The host has requested a read control transfer,
 *                     endpoints are required to be setup in a specific way.
 *                  c) The host has requested a write control transfer, or
 *                     a control data stage is not required, endpoints are
 *                     required to be setup in a specific way.
 *
 *                  Packet processing is resumed by clearing PKTDIS bit.
 *
 * Note:            None
 *****************************************************************************/
void USBCtrlEPServiceComplete(void)
{
    if(ctrl_trf_session_owner == MUID_NULL)
    {
        /*
         * If no one knows how to service this request then stall.
         * Must also prepare EP0 to receive the next SETUP transaction.
         */
        UECONX |= _BV(STALLRQ);        // set stall request 
        UEINTX &= ~_BV(RXSTPI);       // clear Setup interrupt flag
    }
    else    // A module has claimed ownership of the control transfer session.
    {
        if(mRequestDir(bmRequestType) == DEV_TO_HOST)
        {
            if(wLength < wCount)
                wCount = wLength;
            //USBCtrlTrfTxService();
            ctrl_trf_state = CTRL_TRF_TX;
        }
        else    // HOST_TO_DEV
        {
            ctrl_trf_state = CTRL_TRF_RX;
        }
    }

}//end USBCtrlEPServiceComplete

/******************************************************************************
 * Function:        void USBPrepareForNextSetupTrf(void)
 *
 * PreCondition:    None
 *
 * Input:           None
 *
 * Output:          None
 *
 * Side Effects:    None
 *
 * Overview:        The routine forces EP0 OUT to be ready for a new Setup
 *                  transaction, and forces EP0 IN to be owned by CPU.
 *
 * Note:            None
 *****************************************************************************/
void USBPrepareForNextSetupTrf(void)
{
    ctrl_trf_state = WAIT_SETUP;            // See usbctrltrf.h
}//end USBPrepareForNextSetupTrf
