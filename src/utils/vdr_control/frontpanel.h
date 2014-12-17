/*
 * Frontpanel communication for Reelbox PVR1100
 *
 * (c) Georg Acher, acher (at) baycom dot de
 *     BayCom GmbH, http://www.baycom.de
 *
 * #include <gpl-header.h>
 *
 * $Id: vdr_control.c,v 1.1 2004/09/28 00:19:09 acher Exp $
 *
 * Defined User Keys: 
 * User1 IR_TEXT
 * User2 IR_PIP
 * Audio: Shift Less
 * User3 Shift Stop (eject) 
 * User4 Shift Help (epg actual 
 * User5 IR_DVD 
 * User6 IR_DVB
 * User7 IR_PVR
 * User8 IR_REEL
*/

#ifndef _REEL_FRONTPANEL_H
#define _REEL_FRONTPANEL_H

// remote key codes
#define REEL_IR_NONE 0x1000  // no key pressed 
#define REEL_IR_GT	0x5500  // > at numberblock
#define REEL_IR_LT	0x5600  // < at numberblock
#define REEL_IR_0 	0x0000
#define REEL_IR_1 	0x0100
#define REEL_IR_2 	0x0200
#define REEL_IR_3 	0x0300
#define REEL_IR_4 	0x0400
#define REEL_IR_5 	0x0500
#define REEL_IR_6 	0x0600
#define REEL_IR_7 	0x0700
#define REEL_IR_8 	0x0800
#define REEL_IR_9 	0x0900
#define REEL_IR_VOLMINUS 0x0a00
#define REEL_IR_VOLPLUS 0x0b00
#define REEL_IR_MUTE	0x0c00
#define REEL_IR_PROGMINUS 0x0d00
#define REEL_IR_PROGPLUS 0x0e00
#define REEL_IR_STANDBY 0x0f00
#define REEL_IR_MENU	0x2000
#define REEL_IR_UP	0x2100
#define REEL_IR_DOWN	0x2200
#define REEL_IR_LEFT	0x2300
#define REEL_IR_RIGHT	0x2400
#define REEL_IR_OK	0x2500
#define REEL_IR_EXIT	0x2600
// Player Keys
#define REEL_IR_REV	0x3000
#define REEL_IR_PLAY	0x3100
#define REEL_IR_PAUSE	0x3200
#define REEL_IR_FWD	0x3300
#define REEL_IR_REC	0x3500
#define REEL_IR_STOP 	0x3400
// Color Keys
#define REEL_IR_RED	0x4000
#define REEL_IR_GREEN	0x4100
#define REEL_IR_YELLOW	0x4200
#define REEL_IR_BLUE	0x4300
// Device Keys 
#define REEL_IR_DVB     0x5000
#define REEL_IR_TV      0x5100
#define REEL_IR_DVD     0x5200
#define REEL_IR_PVR     0x5300
#define REEL_IR_REEL    0x5400
// Object Keys
#define REEL_IR_SETUP 	0x5700
#define REEL_IR_PIP	0x5800
#define REEL_IR_TEXT	0x5900
#define REEL_IR_HELP	0x5a00	
#define REEL_IR_TIMER	0x5b00
// remote key codes end


// IR device codes
#define REEL_IR_VENDOR 	0x4428
#define REEL_IR_STATE_DVB	0x1d1f
#define REEL_IR_STATE_DVD	0x1b3f
#define REEL_IR_STATE_TV	0x1c2f
#define REEL_IR_STATE_PVR	0x1a4f
#define REEL_IR_STATE_REEL	0x195f

// Frontpanel Keys
#define REEL_KBD_BT0	0x0008 // Standby
#define REEL_KBD_BT1	0x0002 // Red
#define REEL_KBD_BT2	0x0200 // Green
#define REEL_KBD_BT3	0x0020 // Yellow
#define REEL_KBD_BT4	0x0400 // Blue
#define REEL_KBD_BT5	0x0001 // Eject
#define REEL_KBD_BT6	0x0004 // Play
#define REEL_KBD_BT7	0x0080 // Pause
#define REEL_KBD_BT8	0x0800 // Stop
#define REEL_KBD_BT9	0x0040 // Rev
#define REEL_KBD_BT10	0x0010 // Fwd
#define REEL_KBD_BT11	0x0100 // Rec

void fp_noop(void);
void fp_get_version(void);
void fp_enable_messages(int n);
void fp_display_brightness(int n);
void fp_display_contrast(int n);
void fp_clear_display(void);
void fp_write_display(unsigned char *data, int datalen);
void fp_display_data(char *data, int l);
void fp_set_leds(int blink, int state);
void fp_set_clock(void);
void fp_set_wakeup(time_t t);
void fp_set_displaymode(int m);
void fp_set_switchcpu(int timeout);
void fp_get_clock(void);
void fp_set_clock_adjust(int v1, int v2);
void fp_do_display(void);
int fp_open_serial(void);
void fp_close_serial(void);
size_t fp_read(unsigned char *b, size_t l);
int fp_read_msg(unsigned char *msg, int ms);
int u_sleep(long long nsec); 
#endif // _REEL_FRONTPANEL_H

