/************************************************************************************\

  main.c - ATmega32U4 usb to parallel port step/direction driver board

  (c) 2008-2020 Copyright Eckler Software

  Author: David Suffield, dsuffiel@ecklersoft.com

  The host will convert g-code to a step/direction byte array, calculate the acceleration/deceleration
  ramp, generate a step/direction byte array, then write the byte array to the driver board. The driver
  board will buffer the step/direction bytes from the host and clock the bytes out to PORTC.

  The step/direction bytes will be clocked out at a maximum rate of 46875hz.

  The host will handle ABORT command from the user. The ABORT will stop immediately with no valid
  stop position.

  History:

  See makefile.

\************************************************************************************/

#include "usb.h"

struct step_state
{
   union
   {
      uint16_t _word;
      struct
      {
         unsigned abort:1;   /* 1=yes, 0=no */
         unsigned empty:1;   /* 1=yes, 0=no */
         unsigned sync_start:1;   /* 1=yes, 0=no */
         unsigned input0:1;
         unsigned input1:1;
         unsigned input2:1;
         unsigned input3:1;   /* new FW REV-3e */
         unsigned :1;
         unsigned :1;
         unsigned :1;
         unsigned :1;
         unsigned :1;
         unsigned :1;
         unsigned :1;
         unsigned :1;
         unsigned :1;
      };
   };
};

struct step_elements
{
   char reserved[8];
};

struct step_query
{
   struct
   {
      struct step_state state_bits;
      uint16_t icount_period;     /* input0 period in counts */
      uint32_t step;              /* running step count */
   };
};

/* ADC = 8-bit resolution, 16mhz / (64 * 4) = 62.5k clock */
struct step_adc
{
   uint8_t input1;  /* ADCH bits */
   uint8_t input2;
   uint8_t input3;
   uint8_t res;
};

struct step_adc_query
{
   struct
   {
      struct step_state state_bits;
      uint16_t icount_period;     /* input0 period in counts */
      struct step_adc adc;        /* ADC input1-3 */
   };
};

enum ADC_INPUT
{
   ADC_INPUT1=0,
   ADC_INPUT2,
   ADC_INPUT3,
   ADC_INPUT_MAX,
};

struct adc_state
{
   union
   {
      uint8_t _byte;
      struct
      {
         unsigned input1:1;  /* 0=disabled, 1=enabled */
         unsigned input2:1;
         unsigned input3:1;
      };
   };
};

//#define DEBOUNCE_MAX 6
#define DEBOUNCE_MAX 12

static uint32_t step_cnt;
static uint32_t led_cnt;
static uint16_t icount;      /* current input0 count */
static uint16_t last_icount;  /* previous input0 count */
static uint16_t icount_period;    /* input0 frequency */
static uint8_t new_gate;
static uint8_t old_gate;
static uint16_t t0clk;         /* timer0 overflow */
static uint8_t debounce;    /* input0 debounce counter */

static uint8_t bank_val;    /* fifo read value */
static uint16_t bank_cnt;   /* data bank byte count */

static struct step_elements elements;
static struct step_query query_response;
static struct step_state state_bits;
static struct step_adc_query query_response2;
static struct adc_state adc_state_ctrl;

//uint8_t bug;
//uint8_t bug_cnt;
//uint8_t EEMEM ebug_byte = 0xff;    /* use avrdude to init eeprom */
//uint16_t EEMEM ebug_word = 0xffff;
uint16_t EEMEM ebug_line = 0xffff;
//uint8_t EEMEM ebug_buf[8] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff};

/*
 * Primary Oscilator = 16mhz
 * CPU instructon cycle = 16mhz = 63ns, Clock Prescaler Select = 0 (16mhz/1)
 * Timer0 clock = 16mhz
 * Timer0 overflow = 16mhz/256 = 62500hz = 16us
 * rt-test timer0 overhead ~ 7us (verfied with picoscope)
 *
 */

ISR(TIMER0_OVF_vect)
{
   /* Timer0 overflow. */

   /* Select EP */
   UENUM = RT_EP;

   if (state_bits.sync_start == 0)
   {
      if (bit_is_set(UEINTX, FIFOCON))
      {
         if (bit_is_set(UEINTX, RXOUTI))
            UEINTX &= ~_BV(RXOUTI);  /* bank is full, reset OUT interrupt flag */

         /* Unfortunately bank_cnt never gets above 64 bytes. Not even close to 256. */
         MSB(bank_cnt) = UEBCHX;
         LSB(bank_cnt) = UEBCLX;

         if (bank_cnt)
         {
            bank_val = UEDATX;   // read byte from bank
            if (!state_bits.abort)
            {
               mSetDir(DIR_PORT, bank_val);
               mSetStep(STEP_PORT, bank_val);
               step_cnt++;
            }
            bank_cnt--;
         }
         if (bank_cnt == 0)
         {
            UEINTX &= ~_BV(FIFOCON);  /* bank empty, clear FIFOCON */
         }
      }
   }  /* end if (sync_start == 0) */

   /* Check if all banks are free for this EP. */
   state_bits.empty = mBankClear(UESTA0X) ? 1 : 0;

   /* Measure input0 frequency period in counts. */
   t0clk++;
   new_gate = (INPUT0_PORT & _BV(INPUT0_PIN)) ? 1 : 0;
   if (new_gate==0 && old_gate==1)
   {
      if (++debounce >= DEBOUNCE_MAX)
      {
         /* Found gate high to low transistion read 16-bit value. */
         icount = t0clk;
         icount_period = icount - last_icount;
         last_icount = icount;
         debounce = 0;

         /* If syncronized start is enabled, auto reset when step buffers are NOT empty. */
         if (state_bits.sync_start)
            if (!state_bits.empty)
               state_bits.sync_start = 0;
      }
      else
      {
         goto jmpout;  /* complete transistion debounce */
      }
   }
   else
   {
      debounce = 0;
   }
   old_gate = new_gate;

jmpout:
   /* Restore default EP */
   UENUM = EP0_CTRL;

}  /* timer0_handler() */

void bug_blinky(uint16_t line_num, uint8_t cnt)
{
   eeprom_update_word(&ebug_line, line_num);  // note, line_num is hex
   if (cnt == 0)
   {
      while (1)
      {
         BUG_LED_FLIP = _BV(BUG_LED_PIN); // toggle PORTx by writing to PINx (AVR shortcut)
         _delay_ms(150); // wait 0.15s
      }
   }
   else
   {
      while (cnt--)
      {
         BUG_LED_FLIP = _BV(BUG_LED_PIN); // invalid FW
         _delay_ms(150); // wait 0.15s
      }
      BUG_LED_PORT |= _BV(BUG_LED_PIN);  // set LED off
   }
}

static void rt_init(void)
{
   /* Note, init global memory manually (not guaranteed to be zero). */

   state_bits._word = 0;
   step_cnt = 0;
   last_icount = 0;
   icount_period = 0;
   query_response2.adc.input1 = 0;
   query_response2.adc.input2 = 0;
   query_response2.adc.input3 = 0;
   adc_state_ctrl._byte = 0;
   //bug_cnt = 0;
}

static void rt_output0_init(byte cmd)
{
   switch(cmd)
   {
      case OUTPUT_DIGITAL:
         /* Disable pwm */
         TCCR4C &= ~_BV(COM4A0S);  // disable comparator mode for OCR4A
         TCCR4A &= ~_BV(PWM4A);  // disable PWM4A (OUTPUT0)
         /* Enable digital output */
         OUTPUT0_DDR |= _BV(OUTPUT0_PIN); /* set pin to output. */
         OUTPUT0_PORT &= ~_BV(OUTPUT0_PIN); /* clear output pin. */
         break;
      case OUTPUT_PWM:
         /* Enable pwm output */
         OUTPUT0_DDR |= _BV(OUTPUT0_PIN); /* set pin to output. */
         /* Configure timer4 synchronous symmetric pwm. */
         TCCR4D = _BV(WGM40);   // enable phase correct pwm
         TCCR4B = _BV(CS42) | _BV(CS41); // set prescaling, 16mhz / 32 / 256 / 2 = 976.5625hz
         TCCR4C |= _BV(COM4A0S);  // set comparator mode for OCR4A
         TCCR4A |= _BV(PWM4A);  // enable PWM4A (OUTPUT0)
         OUTPUT0_OCR4 = 0x0;  // 0x55 = 30% duty cycle inverted, 0x0 = 5v, 0xff = 0v
         break;
      default:
         break;
   }
}

static void rt_output1_init(byte cmd)
{
   switch(cmd)
   {
      case OUTPUT_DIGITAL:
         /* Disable pwm */
         TCCR4C &= ~_BV(COM4D0);  // disable comparator mode for OCR4D
         TCCR4A &= ~_BV(PWM4D);  // disable PWM4D (OUTPUT1)
         /* Enable digital output */
         OUTPUT1_DDR |= _BV(OUTPUT1_PIN); /* set pin to output. */
         OUTPUT1_PORT &= ~_BV(OUTPUT1_PIN); /* clear output pin. */
         break;
      case OUTPUT_PWM:
         /* Enable pwm output */
         OUTPUT1_DDR |= _BV(OUTPUT1_PIN); /* set pin to output. */
         /* Configure timer4 synchronous symmetric pwm. */
         TCCR4D = _BV(WGM40);   // enable phase correct pwm
         TCCR4B = _BV(CS42) | _BV(CS41); // set prescaling, 16mhz / 32 / 256 / 2 = 976.5625hz
         TCCR4C |= _BV(COM4D0);  // set comparator mode for OCR4D
         TCCR4C |= _BV(PWM4D);  // enable PWM4D (OUTPUT1)
         OUTPUT1_OCR4 = 0x0;  // 0x55 = 30% duty cycle non-inverted, 0x0 = 0v, 0xff = 5v
         break;
      default:
         break;
   }
}

static void rt_output2_init(byte cmd)
{
   switch(cmd)
   {
      case OUTPUT_DIGITAL:
         /* Enable digital output */
         OUTPUT2_DDR |= _BV(OUTPUT2_PIN); /* set pin to output. */
         OUTPUT2_PORT &= ~_BV(OUTPUT2_PIN); /* clear output pin. */
         break;
      default:
         break;
   }
}

static void rt_input0_init(byte cmd)
{
   switch(cmd)
   {
      case INPUT_DIGITAL:
         INPUT0_DDR &= ~_BV(INPUT0_PIN); /* Enable digital input */
         INPUT0_PUP |= _BV(INPUT0_PIN); /* Enable weak pull-up on input. */
         break;
      default:
         break;
   }
}

static void rt_input1_init(byte cmd)
{
   switch(cmd)
   {
      case INPUT_DIGITAL:
         /* Disable ADC input */
         adc_state_ctrl.input1 = 0;
         query_response2.adc.input1 = 0;
         DIDR2 &= ~_BV(ADC11D); /* Enable digital input. */
         /* Enable digital input */
         INPUT1_DDR &= ~_BV(INPUT1_PIN); /* Enable digital input */
         INPUT1_PUP |= _BV(INPUT1_PIN); /* Enable weak pull-up on input. */
         break;
      case INPUT_ADC:
         adc_state_ctrl.input1 = 1;
         INPUT1_DDR &= ~_BV(INPUT1_PIN); /* Enable ADC input */
         INPUT1_PUP &= ~_BV(INPUT1_PIN);  /* Disable weak pull-up on input. */
         ADCSRA = _BV(ADEN) | _BV(ADPS2) | _BV(ADPS1); /* Enable ADC, Prescaler = / 64 */
         ADCSRB |= _BV(MUX5); /* Set bit 5 of analog channel select here. */
         DIDR2 |= _BV(ADC11D); /* Disable digital input. This saves power. */
         break;
      default:
         break;
   }
}

static void rt_input2_init(byte cmd)
{
   switch(cmd)
   {
      case INPUT_DIGITAL:
         /* Disable ADC input */
         adc_state_ctrl.input2 = 0;
         query_response2.adc.input2 = 0;
         DIDR2 &= ~_BV(ADC12D); /* Enable digital input. */
         /* Enable digital input */
         INPUT2_DDR &= ~_BV(INPUT2_PIN); /* Enable digital input */
         INPUT2_PUP |= _BV(INPUT2_PIN); /* Enable weak pull-up on input. */
         break;
      case INPUT_ADC:
         adc_state_ctrl.input2 = 1;
         INPUT2_DDR &= ~_BV(INPUT2_PIN); /* Enable ADC input */
         INPUT2_PUP &= ~_BV(INPUT2_PIN);  /* Disable weak pull-up on input. */
         ADCSRA = _BV(ADEN) | _BV(ADPS2) | _BV(ADPS1); /* Enable ADC, Prescaler = / 64 */
         ADCSRB |= _BV(MUX5); /* Set bit 5 of analog channel select here. */
         DIDR2 |= _BV(ADC12D); /* Disable digital input. This saves power. */
         break;
      default:
         break;
   }
}

static void rt_input3_init(byte cmd)
{
   switch(cmd)
   {
      case INPUT_DIGITAL:
         /* Disable ADC input */
         adc_state_ctrl.input3 = 0;
         query_response2.adc.input3 = 0;
         DIDR2 &= ~_BV(ADC13D); /* Enable digital input. */
         /* Enable digital input */
         INPUT3_DDR &= ~_BV(INPUT3_PIN); /* Enable digital input */
         INPUT3_PUP |= _BV(INPUT3_PIN); /* Enable weak pull-up on input. */
         break;
      case INPUT_ADC:
         adc_state_ctrl.input3 = 1;
         INPUT3_DDR &= ~_BV(INPUT3_PIN); /* Enable ADC input */
         INPUT3_PUP &= ~_BV(INPUT3_PIN);  /* Disable weak pull-up on input. */
         ADCSRA = _BV(ADEN) | _BV(ADPS2) | _BV(ADPS1); /* Enable ADC, Prescaler = / 64 */
         ADCSRB |= _BV(MUX5); /* Set bit 5 of analog channel select here. */
         DIDR2 |= _BV(ADC13D); /* Disable digital input. This saves power. */
         break;
      default:
         break;
   }
}

/* Class specific callback for EP0 Setup requests. Called when no other module in the firmware framework owns the request. */
void rt_check_request(void)
{
   if(mRequestType(bmRequestType) != VENDOR)
      return;

   switch(bRequest)
   {
      case STEP_ABORT_SET:
         state_bits.abort = 1;
         ctrl_trf_session_owner = MUID_RT;
         UEINTX &= ~_BV(RXSTPI);       // clear Setup interrupt flag
         UEINTX &= ~_BV(TXINI);       // send IN 0 bytes status stage
         break;
      case STEP_ABORT_CLEAR:
         state_bits.abort = 0;
         ctrl_trf_session_owner = MUID_RT;
         UEINTX &= ~_BV(RXSTPI);       // clear Setup interrupt flag
         UEINTX &= ~_BV(TXINI);       // send IN 0 bytes status stage
         break;
      case STEP_QUERY:
         query_response.state_bits._word = state_bits._word;
         query_response.state_bits.input0 = (INPUT0_PORT & _BV(INPUT0_PIN)) ? 1 : 0;
         query_response.state_bits.input1 = (INPUT1_PORT & _BV(INPUT1_PIN)) ? 1 : 0;
         query_response.state_bits.input2 = (INPUT2_PORT & _BV(INPUT2_PIN)) ? 1 : 0;
         query_response.state_bits.input3 = (INPUT3_PORT & _BV(INPUT3_PIN)) ? 1 : 0;
         query_response.icount_period = icount_period;
         query_response.step = step_cnt;
         ctrl_trf_session_owner = MUID_RT;
         pSrc = &query_response;  /* set source for next IN session */
         ctrl_trf_mem = _RAM;               // Set memory type
         wCount = sizeof(query_response);      // Set data count
         UEINTX &= ~_BV(RXSTPI);       // clear Setup interrupt flag
         break;
      case STEP_ADC_QUERY:
         query_response2.state_bits._word = state_bits._word;
         query_response2.state_bits.input0 = (INPUT0_PORT & _BV(INPUT0_PIN)) ? 1 : 0;
         query_response2.state_bits.input1 = (INPUT1_PORT & _BV(INPUT1_PIN)) ? 1 : 0;
         query_response2.state_bits.input2 = (INPUT2_PORT & _BV(INPUT2_PIN)) ? 1 : 0;
         query_response2.state_bits.input3 = (INPUT3_PORT & _BV(INPUT3_PIN)) ? 1 : 0;
         query_response2.icount_period = icount_period;
         ctrl_trf_session_owner = MUID_RT;
         pSrc = &query_response2;  /* set source for next IN session */
         ctrl_trf_mem = _RAM;               // Set memory type
         wCount = sizeof(query_response2);      // Set data count
         UEINTX &= ~_BV(RXSTPI);       // clear Setup interrupt flag
         break;
      case STEP_OUTPUT0_SET:
         OUTPUT0_PORT |= _BV(OUTPUT0_PIN);
         ctrl_trf_session_owner = MUID_RT;
         UEINTX &= ~_BV(RXSTPI);       // clear Setup interrupt flag
         UEINTX &= ~_BV(TXINI);       // send IN 0 bytes status stage
         break;
      case STEP_OUTPUT0_CLEAR:
         OUTPUT0_PORT &= ~_BV(OUTPUT0_PIN);
         ctrl_trf_session_owner = MUID_RT;
         UEINTX &= ~_BV(RXSTPI);       // clear Setup interrupt flag
         UEINTX &= ~_BV(TXINI);       // send IN 0 bytes status stage
         break;
      case STEP_OUTPUT1_SET:
         OUTPUT1_PORT |= _BV(OUTPUT1_PIN);
         ctrl_trf_session_owner = MUID_RT;
         UEINTX &= ~_BV(RXSTPI);       // clear Setup interrupt flag
         UEINTX &= ~_BV(TXINI);       // send IN 0 bytes status stage
         break;
      case STEP_OUTPUT1_CLEAR:
         OUTPUT1_PORT &= ~_BV(OUTPUT1_PIN);
         ctrl_trf_session_owner = MUID_RT;
         UEINTX &= ~_BV(RXSTPI);       // clear Setup interrupt flag
         UEINTX &= ~_BV(TXINI);       // send IN 0 bytes status stage
         break;
      case STEP_OUTPUT2_SET:
         OUTPUT2_PORT |= _BV(OUTPUT2_PIN);
         ctrl_trf_session_owner = MUID_RT;
         UEINTX &= ~_BV(RXSTPI);       // clear Setup interrupt flag
         UEINTX &= ~_BV(TXINI);       // send IN 0 bytes status stage
         break;
      case STEP_OUTPUT2_CLEAR:
         OUTPUT2_PORT &= ~_BV(OUTPUT2_PIN);
         ctrl_trf_session_owner = MUID_RT;
         UEINTX &= ~_BV(RXSTPI);       // clear Setup interrupt flag
         UEINTX &= ~_BV(TXINI);       // send IN 0 bytes status stage
         break;
      case STEP_SYNC_START_SET:
         state_bits.sync_start = 1; /* enable synchronized transfer using input0 index pulse */
         ctrl_trf_session_owner = MUID_RT;
         UEINTX &= ~_BV(RXSTPI);       // clear Setup interrupt flag
         UEINTX &= ~_BV(TXINI);       // send IN 0 bytes status stage
         break;
      case STEP_OUTPUT0_MODE:
         rt_output0_init(wValue);
         ctrl_trf_session_owner = MUID_RT;
         UEINTX &= ~_BV(RXSTPI);       // clear Setup interrupt flag
         UEINTX &= ~_BV(TXINI);       // send IN 0 bytes status stage
         break;
      case STEP_OUTPUT0_PWM:
         OUTPUT0_OCR4 = wValue;
         ctrl_trf_session_owner = MUID_RT;
         UEINTX &= ~_BV(RXSTPI);       // clear Setup interrupt flag
         UEINTX &= ~_BV(TXINI);       // send IN 0 bytes status stage
         break;
      case STEP_OUTPUT1_MODE:
         rt_output1_init(wValue);
         ctrl_trf_session_owner = MUID_RT;
         UEINTX &= ~_BV(RXSTPI);       // clear Setup interrupt flag
         UEINTX &= ~_BV(TXINI);       // send IN 0 bytes status stage
         break;
      case STEP_OUTPUT1_PWM:
         OUTPUT1_OCR4 = wValue;
         ctrl_trf_session_owner = MUID_RT;
         UEINTX &= ~_BV(RXSTPI);       // clear Setup interrupt flag
         UEINTX &= ~_BV(TXINI);       // send IN 0 bytes status stage
         break;
      case STEP_INPUT1_MODE:
         rt_input1_init(wValue);
         ctrl_trf_session_owner = MUID_RT;
         UEINTX &= ~_BV(RXSTPI);       // clear Setup interrupt flag
         UEINTX &= ~_BV(TXINI);       // send IN 0 bytes status stage
         break;
      case STEP_INPUT2_MODE:
         rt_input2_init(wValue);
         ctrl_trf_session_owner = MUID_RT;
         UEINTX &= ~_BV(RXSTPI);       // clear Setup interrupt flag
         UEINTX &= ~_BV(TXINI);       // send IN 0 bytes status stage
         break;
      case STEP_INPUT3_MODE:
         rt_input3_init(wValue);
         ctrl_trf_session_owner = MUID_RT;
         UEINTX &= ~_BV(RXSTPI);       // clear Setup interrupt flag
         UEINTX &= ~_BV(TXINI);       // send IN 0 bytes status stage
         break;
      case STEP_SET:
         rt_init();
         pDst = &elements;  /* set destination for next OUT session */
         ctrl_trf_mem = _RAM;               // Set memory type
         wCount = sizeof(elements);            // Set data count
         ctrl_trf_session_owner = MUID_RT;
         UEINTX &= ~_BV(RXSTPI);       // clear Setup interrupt flag
         break;
      default:
         break;
    }
}  /* rt_check_request() */

/*
 * Class specific callback for initializing endpoints, buffer descriptors, internal state-machine, and variables.
 * Called after the USB host has sent a SET_CONFIGURATION request.
 */
void rt_init_ep(void)
{
   rt_init();
   LED_PORT &= ~_BV(LED_PIN);       /* Enumerated, set LED on. */

   /* Initialize control EP1 */
   UENUM = RT_EP;  /* select EP */
   UECONX |= _BV(EPEN); /* activate EP */
   UECFG0X = _BV(EPTYPE1);  /* EPTYPE=Bulk, EPDIR=OUT */
   // Although EPSIZE > 64 is settable the AVR USB controller only uses 64 bytes max. DES 5/13/2020 
   //UECFG1X = _BV(EPSIZE2) | _BV(EPSIZE0) | _BV(ALLOC); /* EPSIZE=256 bytes, EPBK=1 bank */
   UECFG1X = _BV(EPSIZE2) | _BV(EPSIZE0) | _BV(EPBK0) | _BV(ALLOC); /* EPSIZE=64 bytes, EPBK=2 banks */
   if (bit_is_clear(UESTA0X, CFGOK))
      bug_blinky(__LINE__, 0);  /* EP config error */

   UENUM = EP0_CTRL;  /* select EP */

   /* Enable interrupts now if valid FW for this AVR */
   if (sn_check_ok)
      sei();

   UEINTX &= ~_BV(RXSTPI);       // clear Setup interrupt flag
   UEINTX &= ~_BV(TXINI);       // send IN 0 bytes status stage
}

static void rt_class_service(void)
{
   /* Nothing to do, see if device has enumerated. */
   if (usb_device_state != CONFIGURED_STATE)
   {
      /* No enumeration, blink LED. */
      led_cnt++;
      if (led_cnt > 40160)
      {
         LED_FLIP = _BV(LED_PIN);  // toggle LED_PIN
         led_cnt=0;
      }
     return;
   }

   if (!sn_check_ok)
   {
      led_cnt++;
      if (led_cnt > 40160)
      {
         BUG_LED_FLIP = _BV(BUG_LED_PIN); // invalid FW
         led_cnt=0;
      }
      return;
   }

   /* If ADC is enabled process analog to digital channel conversion(s). */
   if (adc_state_ctrl._byte)
   {
      /* If conversion is complete. Store result. */
      if (bit_is_clear(ADCSRA, ADSC))
      {
         switch (ADMUX)
         {
            case INPUT1_ADC:
               if (adc_state_ctrl.input1)
                  query_response2.adc.input1 = ADCH;  // save adc result
               ADMUX = INPUT2_ADC;  // select next adc channel
               break;
            case INPUT2_ADC:
               if (adc_state_ctrl.input2)
                  query_response2.adc.input2 = ADCH;  // save adc result
               ADMUX = INPUT3_ADC;  // select next adc channel
               break;
            case INPUT3_ADC:
               if (adc_state_ctrl.input3)
                  query_response2.adc.input3 = ADCH;  // save adc result
               ADMUX = INPUT1_ADC;  // select next adc channel
               break;
            default:
               ADMUX = INPUT1_ADC;  // nothing selected, kick start
               break;
         }
         ADCSRA |= _BV(ADSC);  // start new conversion
      }
   }
}  /* rt_class_service() */

int main(void)
{
   /* Make sure all interrupts are disabled. */
   cli();

   /* Set led bits as output */
   LED_DDR |= _BV(LED_PIN);
   BUG_LED_DDR |= _BV(BUG_LED_PIN);
   BUG_LED_PORT |= _BV(BUG_LED_PIN);  // set LED off

   /* Set Step/Direction bits as outputs */
   mSetDir(DIR_DDR, 0xff);
   mSetStep(STEP_DDR, 0xff);

   /*
    * Output0-1 must be configured from software, they are tri-stated on reset.
    */

   /* Set output2 digital and logic zero. */
   rt_output2_init(OUTPUT_DIGITAL);

#if 0
   /* Configure timer4 synchronous symmetric pwm for OUTPUT0-1. */
   TCCR4D = _BV(WGM40);   // enable phase correct pwm
   TCCR4B = _BV(CS42) | _BV(CS41); // set prescaling, 16mhz / 32 / 256 / 2 = 976.5625hz
   TCCR4C = _BV(COM4A0S) | _BV(COM4D0);  // set comparator modes for OCRA, OCRD
   TCCR4A |= _BV(PWM4A);  // enable PWM4A (OUTPUT0)
   TCCR4C |= _BV(PWM4D);  // enable PWM4D (OUTPUT1)

   OUTPUT0_OCR4 = 0x55;  // 30% duty cycle inverted, 0x0 = 5v, 0xff = 0v
   OUTPUT1_OCR4 = 0x55;  // 30% duty cycle, 0x0 = 0v, 0xff = 5v
#endif

   /* Set input0 digital with weak pullup. */
   rt_input0_init(INPUT_DIGITAL);

   /*
    * Input1-3 must be configured from software, they are tri-stated on reset.
    */

   /* Configure timer0 */
   TCCR0A = 0;            // enable normal mode
   TCCR0B |= _BV(CS00);  // start clock, CS0 = no prescaling

   /* Clear and enable timer0 overflow interrupt */
   TIFR0 &= ~_BV(TOV0);
   TIMSK0 |= _BV(TOIE0);

   USBDriverInitialize();         // See usbdrv.c

   while (1)
   {
      /* Process standard EP0 transactions (enumeration, descriptors, EP0 Setup, EP0 In, EP0 Out). */
      USBCheckBusStatus();    // See usbdrv.c
      USBDriverService();     // "
      rt_class_service();
   }
}
