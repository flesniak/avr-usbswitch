/* Name: main.c
 * Project: hid-data, example how to use HID for data transfer
 * Author: Christian Starkjohann
 * Creation Date: 2008-04-11
 * Tabsize: 4
 * Copyright: (c) 2008 by OBJECTIVE DEVELOPMENT Software GmbH
 * License: GNU GPL v2 (see License.txt), GNU GPL v3 or proprietary (CommercialLicense.txt)
 */

#include <avr/io.h>
#include <avr/wdt.h>
#include <avr/interrupt.h>  /* for sei() */
#include <util/delay.h>     /* for _delay_ms() */
#include <avr/eeprom.h>
#include <stdbool.h>

#include <avr/pgmspace.h>   /* required by usbdrv.h */
#include "usbdrv.h"
#include "oddebug.h"        /* This is also an example for using debug macros */

/* ------------------------------------------------------------------------- */
/* ----------------------------- USB interface ----------------------------- */
/* ------------------------------------------------------------------------- */

PROGMEM const char usbHidReportDescriptor[22] = {    /* USB report descriptor */
    0x06, 0x00, 0xff,              // USAGE_PAGE (Generic Desktop)
    0x09, 0x01,                    // USAGE (Vendor Usage 1)
    0xa1, 0x01,                    // COLLECTION (Application)
    0x15, 0x00,                    //   LOGICAL_MINIMUM (0)
    0x26, 0xff, 0x00,              //   LOGICAL_MAXIMUM (255)
    0x75, 0x08,                    //   REPORT_SIZE (8)
    0x95, 0x80,                    //   REPORT_COUNT (128)
    0x09, 0x00,                    //   USAGE (Undefined)
    0xb2, 0x02, 0x01,              //   FEATURE (Data,Var,Abs,Buf)
    0xc0                           // END_COLLECTION
};
/* Since we define only one feature report, we don't use report-IDs (which
 * would be the first byte of the report). The entire report consists of 128
 * opaque data bytes.
 */

/* ------------------------------------------------------------------------- */

static uchar bytesRemaining = 0;
static bool debounce_wait = 0;

#define LED_ON  (PORTB |=  (1 << PB2)) // active-high
#define LED_OFF (PORTB &= ~(1 << PB2))
#define LED_TOG (PORTB ^=  (1 << PB2))

#define SWITCH_ON  (PORTA &= ~(1 << PA7)) // active-low
#define SWITCH_OFF (PORTA |=  (1 << PA7))
#define SWITCH_TOG (PORTA ^=  (1 << PA7))
#define SWITCH_STA (!((PINA >> PA7) & 1))

#define BUTTON (!(PINA & (1 << PA4))) // active-low

/* usbFunctionRead() is called when the host requests a chunk of data from
 * the device. For more information see the documentation in usbdrv/usbdrv.h.
 */
uchar usbFunctionRead(uchar *data, uchar len) {
    data[0] = SWITCH_STA;
    return 1;
}

/* usbFunctionWrite() is called when the host sends a chunk of data to the
 * device. For more information see the documentation in usbdrv/usbdrv.h.
 */
uchar usbFunctionWrite(uchar *data, uchar len) {
    // PORTA = (PORTA & ~(1 << PA0)) | ((data[0] & 1) << PA0);
    if (bytesRemaining > 0) {
        LED_ON;
        if (data[0] & 1)
            SWITCH_ON;
        else
            SWITCH_OFF;
        bytesRemaining--;
    }
    return len;
}

/* ------------------------------------------------------------------------- */

usbMsgLen_t usbFunctionSetup(uchar data[8]) {
    usbRequest_t *rq = (void *)data;

    if ((rq->bmRequestType & USBRQ_TYPE_MASK) == USBRQ_TYPE_CLASS) {    /* HID class request */
        if (rq->bRequest == USBRQ_HID_GET_REPORT){  /* wValue: ReportType (highbyte), ReportID (lowbyte) */
            /* since we have only one report type, we can ignore the report-ID */
            return USB_NO_MSG;  /* use usbFunctionRead() to obtain data */
        } else if (rq->bRequest == USBRQ_HID_SET_REPORT) {
            bytesRemaining = 1; // writing a single byte is sufficient
            /* since we have only one report type, we can ignore the report-ID */
            return USB_NO_MSG;  /* use usbFunctionWrite() to receive data from host */
        }
    } else {
        /* ignore vendor type requests, we don't use any */
    }
    return 0;
}

/* ------------------------------------------------------------------------- */

ISR(TIM1_COMPA_vect) {
    LED_OFF;

    if (debounce_wait && !BUTTON)
        debounce_wait = false;
}

int main(void) {
    uchar i;

    wdt_enable(WDTO_1S);
    /* Even if you don't use the watchdog, turn it off here. On newer devices,
     * the status of the watchdog (on/off, period) is PRESERVED OVER RESET!
     */

    DDRA = (1 << PA7); // only output for switch
    PORTA = 0b11110011; // pullups on all inputs (except USB), power off switch
    DDRB = (1 << PB2); // hello world led
    PORTB = 0xff;

    // hello world led
	OCR1A = 2441; //set compare match value ~ 8hz
	TCCR1A |= (0 << WGM11) | (0 << WGM10);
	TCCR1B |= (0 << WGM13) | (1 << WGM12) | (1 << CS12) | (0 << CS11) | (1 << CS10); // prescaler 1024, clear on compare match
	TIMSK1 |= (1 << OCIE1A); // compare match on OCR1A

    usbInit();
    usbDeviceDisconnect();  /* enforce re-enumeration, do this while interrupts are disabled! */
    i = 0;
    while(--i) {            /* fake USB disconnect for > 250 ms */
        wdt_reset();
        _delay_ms(1);
    }

    usbDeviceConnect();
    sei();
    for(;;) {
        wdt_reset();
        usbPoll();

        if (!debounce_wait && BUTTON) {
            LED_ON;
            SWITCH_TOG;
            debounce_wait = true;
        }
    }
    return 0;
}
