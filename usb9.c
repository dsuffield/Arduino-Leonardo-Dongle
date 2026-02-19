/************************************************************************************\

  usb9.c - ATmega32U4 USB driver

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

void USBStdGetDscHandler(void);
void USBStdSetCfgHandler(void);
void USBStdGetStatusHandler(void);
void USBStdFeatureReqHandler(void);
void USBVendorGetOSHandler(void);

/******************************************************************************
 * Function:        void USBCheckStdRequest(void)
 *
 * PreCondition:    None
 *
 * Input:           None
 *
 * Output:          None
 *
 * Side Effects:    None
 *
 * Overview:        This routine checks the setup data packet to see if it
 *                  knows how to handle it
 *
 * Note:            None
 *****************************************************************************/
void USBCheckStdRequest(void)
{   
    if(mRequestType(bmRequestType) != STANDARD) return;
    
    switch(bRequest)
    {
        case SET_ADR:
            ctrl_trf_session_owner = MUID_USB9;
            usb_device_state = ADR_PENDING_STATE;       // Update state
            UDADDR = wValue & ~_BV(ADDEN);        // Save, but don't enable
            UEINTX &= ~_BV(RXSTPI);       // clear Setup interrupt flag
            UEINTX &= ~_BV(TXINI);       // send IN 0 bytes status stage
            /* See USBCtrlTrfInHandler() in usbctrltrf.c for the next step */
            break;
        case GET_DSC:
            USBStdGetDscHandler();
            break;
        case SET_CFG:
            USBStdSetCfgHandler();
            break;
        case GET_CFG:
            ctrl_trf_session_owner = MUID_USB9;
            pSrc = &usb_active_cfg;         // Set Source
            ctrl_trf_mem = _RAM;               // Set memory type
            wCount = 1;                            // Set data count
            UEINTX &= ~_BV(RXSTPI);       // clear Setup interrupt flag
            break;
        case GET_STATUS:
            USBStdGetStatusHandler();
            break;
        case CLR_FEATURE:
        case SET_FEATURE:
            USBStdFeatureReqHandler();
            break;
        case GET_INTF:
            ctrl_trf_session_owner = MUID_USB9;
            pSrc = &usb_alt_intf + (wIndex & 0xff);  // Set source
            ctrl_trf_mem = _RAM;               // Set memory type
            wCount = 1;                            // Set data count
            UEINTX &= ~_BV(RXSTPI);       // clear Setup interrupt flag
            break;
        case SET_INTF:
            ctrl_trf_session_owner = MUID_USB9;
            usb_alt_intf[wIndex] = (byte)wValue & 0xff;
            UEINTX &= ~_BV(RXSTPI);       // clear Setup interrupt flag
            break;
        case SET_DSC:
        case SYNCH_FRAME:
        default:
            break;
    }//end switch
    
}//end USBCheckStdRequest

/******************************************************************************
 * Function:        void USBStdGetDscHandler(void)
 *
 * PreCondition:    None
 *
 * Input:           None
 *
 * Output:          None
 *
 * Side Effects:    None
 *
 * Overview:        This routine handles the standard GET_DESCRIPTOR request.
 *                  It utilizes tables dynamically looks up descriptor size.
 *                  This routine should never have to be modified if the tables
 *                  in usbdsc.c are declared correctly.
 *
 * Note:            None
 *****************************************************************************/
void USBStdGetDscHandler(void)
{
    byte dsc_type = MSB(wValue);
    byte dsc_index = LSB(wValue);

    if(mRequestDir(bmRequestType) == DEV_TO_HOST)
    {
        switch(dsc_type)
        {
            case DSC_DEV:
                ctrl_trf_session_owner = MUID_USB9;
                pSrc = &device_dsc;
                ctrl_trf_mem = _ROM;                       // Set memory type
                wCount = sizeof(device_dsc);          // Set data count
                UEINTX &= ~_BV(RXSTPI);       // clear Setup interrupt flag
                break;
            case DSC_CFG:
                ctrl_trf_session_owner = MUID_USB9;
                pSrc = &cfg01;
                ctrl_trf_mem = _ROM;                       // Set memory type
                wCount = sizeof(cfg01);              // Set data count
                UEINTX &= ~_BV(RXSTPI);       // clear Setup interrupt flag
                break;
            case DSC_STR:
                switch (dsc_index)
                {
                    case LANG_INDEX:
                        ctrl_trf_session_owner = MUID_USB9;
                        pSrc = &sd000;
                        ctrl_trf_mem = _ROM;                       // Set memory type
                        wCount = sizeof(sd000);
                        UEINTX &= ~_BV(RXSTPI);       // clear Setup interrupt flag
                        break;
                    case MAN_INDEX:
                        ctrl_trf_session_owner = MUID_USB9;
                        pSrc = &sd001;
                        ctrl_trf_mem = _ROM;                       // Set memory type
                        wCount = sizeof(sd001);
                        UEINTX &= ~_BV(RXSTPI);       // clear Setup interrupt flag
                        break;
                    case PROD_INDEX:
                        ctrl_trf_session_owner = MUID_USB9;
                        pSrc = &sd002;
                        ctrl_trf_mem = _ROM;                       // Set memory type
                        wCount = sizeof(sd002);
                        UEINTX &= ~_BV(RXSTPI);       // clear Setup interrupt flag
                        break;
                    case SN_INDEX:
                        ctrl_trf_session_owner = MUID_USB9;
                        if (sn_check_ok)
                        {
                            pSrc = &sd003;             // valid FW use date code sn
                            ctrl_trf_mem = _ROM;       // Set memory type
                            wCount = sizeof(sd003);
                        }
                        else
                        {
                            pSrc = &avr_sn;            // invalid FW use AVR sn
                            ctrl_trf_mem = _RAM;       // Set memory type
                            wCount = sizeof(avr_sn);
                        }
                        UEINTX &= ~_BV(RXSTPI);       // clear Setup interrupt flag
                        break;
                    case MICROSOFT_OS_INDEX:
                        ctrl_trf_session_owner = MUID_USB9;
                        pSrc = &usb_sd_os;
                        ctrl_trf_mem = _ROM;                       // Set memory type
                        wCount = sizeof(usb_sd_os);
                        UEINTX &= ~_BV(RXSTPI);       // clear Setup interrupt flag
                        break;
                    default:
                        break;
                }
                break;
            default:
                break;
        }//end switch
    }//end if
}//end USBStdGetDscHandler

/******************************************************************************
 * Function:        void USBStdSetCfgHandler(void)
 *
 * PreCondition:    None
 *
 * Input:           None
 *
 * Output:          None
 *
 * Side Effects:    None
 *
 * Overview:        This routine first disables all endpoints by clearing
 *                  UEP registers. It then configures (initializes) endpoints
 *                  specified in the modifiable section.
 *
 * Note:            None
 *****************************************************************************/
void USBStdSetCfgHandler(void)
{
    byte cfg_value = LSB(wValue);
    byte i;

    ctrl_trf_session_owner = MUID_USB9;

    /* Reset EP1-EP7 */
    UERST |= _BV(EPRST6) | _BV(EPRST5) | _BV(EPRST4) | _BV(EPRST3) | _BV(EPRST2) | _BV(EPRST1);

    /* Clear EP1-EP6 reset */
    UERST &= ~(_BV(EPRST6) | _BV(EPRST5) | _BV(EPRST4) | _BV(EPRST3) | _BV(EPRST2) | _BV(EPRST1));

    for (i=0; i<MAX_NUM_INT; i++)
        usb_alt_intf[i] = 0;

    usb_active_cfg = cfg_value;
    if(cfg_value == 0)
        usb_device_state = ADDRESS_STATE;
    else
    {
        usb_device_state = CONFIGURED_STATE;

        /* Modifiable Section */
        rt_init_ep();
        /* End modifiable section */

    }//end if(bdt_data.SetupPkt.bcfgValue == 0)

}//end USBStdSetCfgHandler

/******************************************************************************
 * Function:        void USBStdGetStatusHandler(void)
 *
 * PreCondition:    None
 *
 * Input:           None
 *
 * Output:          None
 *
 * Side Effects:    None
 *
 * Overview:        This routine handles the standard GET_STATUS request
 *
 * Note:            None
 *****************************************************************************/
void USBStdGetStatusHandler(void)
{
    CtrlTrfData[0] = 0;                         // Initialize content
    CtrlTrfData[1] = 0;
        
    switch(mRequestRecp(bmRequestType))
    {
        case RCPT_DEV:
            ctrl_trf_session_owner = MUID_USB9;
            /*
             * _byte0: bit0: Self-Powered Status [0] Bus-Powered [1] Self-Powered
             *         bit1: RemoteWakeup        [0] Disabled    [1] Enabled
             */
// No self_power. DES 2/22/09
//            if(self_power == 1)                     // self_power defined in io_cfg.h
//                bdt_data.CtrlTrfData._byte0|=0x1;    // Set bit0
            
            //if(usb_stat.RemoteWakeup == 1)          // usb_stat defined in usbmmap.c
            //    CtrlTrfData._byte0|=0x2;     // Set bit1
            break;
        case RCPT_INTF:
            ctrl_trf_session_owner = MUID_USB9;     // No data to update
            break;
        case RCPT_EP:
            ctrl_trf_session_owner = MUID_USB9;
            /*
             * _byte0: bit0: Halt Status [0] Not Halted [1] Halted
             */
            //pDst = &bdt_data.ep_bd_pairs[0].ep_bd_out+(bdt_data.SetupPkt.EPNum*8)+(bdt_data.SetupPkt.EPDir*4);
            //if(*pDst & _BSTALL)    // Use _BSTALL as a bit mask
            //    bdt_data.CtrlTrfData._byte0=0x01;// Set bit0
            break;
    }//end switch
    
    if(ctrl_trf_session_owner == MUID_USB9)
    {
        pSrc = &CtrlTrfData;            // Set Source
        ctrl_trf_mem = _RAM;               // Set memory type
        wCount = 2;                            // Set data count
        UEINTX &= ~_BV(RXSTPI);       // clear Setup interrupt flag
    }//end if(...)
}//end USBStdGetStatusHandler

/******************************************************************************
 * Function:        void USBStdFeatureReqHandler(void)
 *
 * PreCondition:    None
 *
 * Input:           None
 *
 * Output:          None
 *
 * Side Effects:    None
 *
 * Overview:        This routine handles the standard SET & CLEAR FEATURES
 *                  requests
 *
 * Note:            None
 *****************************************************************************/
void USBStdFeatureReqHandler(void)
{
#if 0
    if((bdt_data.SetupPkt.bFeature == DEVICE_REMOTE_WAKEUP)&&
       (bdt_data.SetupPkt.Recipient == RCPT_DEV))
    {
        ctrl_trf_session_owner = MUID_USB9;
        if(bdt_data.SetupPkt.bRequest == SET_FEATURE)
            usb_stat.RemoteWakeup = 1;
        else
            usb_stat.RemoteWakeup = 0;
    }//end if
    
    if((bdt_data.SetupPkt.bFeature == ENDPOINT_HALT)&&
       (bdt_data.SetupPkt.Recipient == RCPT_EP)&&
       (bdt_data.SetupPkt.EPNum != 0))
    {
        ctrl_trf_session_owner = MUID_USB9;
        /* Must do address calculation here */
        pDst = (__near unsigned char *)&bdt_data.ep_bd_pairs[0].ep_bd_out+(bdt_data.SetupPkt.EPNum*8)+(bdt_data.SetupPkt.EPDir*4);
        
        if(bdt_data.SetupPkt.bRequest == SET_FEATURE)
            *pDst = _USIE|_BSTALL;
        else
        {
            if(bdt_data.SetupPkt.EPDir == 1) // IN
                *pDst = _UCPU;
            else
                *pDst = _USIE|_DAT0|_DTSEN;
        }//end if
    }//end if
#endif
}//end USBStdFeatureReqHandler

/********************************************************************
 * Function:        void USBCheckVendorRequest(void)
 *
 * PreCondition:    None
 *
 * Input:           None
 *
 * Output:          None
 *
 * Side Effects:    None
 *
 * Overview:        This routine checks the setup data packet to see
 *                  if it knows how to handle it
 *
 * Note:            None
 *******************************************************************/
void USBCheckVendorRequest(void)
{
    if(mRequestType(bmRequestType) != VENDOR) return;

    switch(bRequest)
    {
        case MS_OS_VENDOR_CODE:
            USBVendorGetOSHandler();
            break;
        case SYNCH_FRAME:
        default:
            break;
    }//end switch
}//end USBCheckVendorRequest

/********************************************************************
 * Function:        void USBVendorGetOSHandler(void)
 *
 * PreCondition:    None
 *
 * Input:           None
 *
 * Output:          None
 *
 * Side Effects:    None
 *
 * Overview:        This routine handles OS Vendor Request for MS/WinUSB
 *
 * Note:            None
 *******************************************************************/
void USBVendorGetOSHandler(void)
{
    if(mRequestDir(bmRequestType) == DEV_TO_HOST)
    {
        switch(wIndex)
        {
            case 0x04:
                ctrl_trf_session_owner = MUID_USB9;
                pSrc = &ext_cid_os_fd;
                wCount = sizeof(ext_cid_os_fd);
                ctrl_trf_mem = _ROM;                       // Set memory type
                UEINTX &= ~_BV(RXSTPI);       // clear Setup interrupt flag
                break;                   
            case 0x05:
                ctrl_trf_session_owner = MUID_USB9;
                pSrc = &ext_p_os_fd;
                wCount = sizeof(ext_p_os_fd);
                ctrl_trf_mem = _ROM;                       // Set memory type
                UEINTX &= ~_BV(RXSTPI);       // clear Setup interrupt flag
                break;
        }//end switch
    }//end if
}//end USBVendorGetOSHandler

