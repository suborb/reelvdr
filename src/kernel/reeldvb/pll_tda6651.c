/*****************************************************************************/
/*
 * TDA6651-PLL driver for Philips TU1216
 * 
 * (c) 2005 Georg Acher, BayCom GmbH (acher at baycom dot de)
 *
 * Extracted from dvb-ttusb-budget.c, fixed PLL calculation
 *
 * (c) by
 * Holger Waechtler <holger@convergence.de>
 * Felix Domke <tmbinc@elitedvb.net>
 *
 * #include <gpl-header>
 *
 * $Id: pll_tda6651.c,v 1.2 2006/01/29 22:23:11 acher Exp $
 */
/*****************************************************************************/

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/string.h>
#include <linux/slab.h>
#include <asm/div64.h>
#include <linux/firmware.h>

#include "dvb_frontend.h"

#include "tda1004x.h"

#define CB1A_NO_ALBC (0xc0+0x8+0x2)  // T210 R210
#define CB1A_ALBC (0xc0+0x18+0x2)  // T210 R210
#define CB1B (0x80+8+2)       // ATC / AL210
#define CB2 0xab              // CP210 BS54321
/* ------------------------------------------------------------------------- */

static int philips_tu1216_pll_init(struct dvb_frontend* fe)
{
	struct reel_unit* ru = (struct reel_unit*) fe->dvb->priv;
	u8 td1316_init[] = { 0x0b, 0xf5, CB1A_NO_ALBC, CB2, CB1B, CB2 };
	struct i2c_msg tuner_msg = { .addr=0x60, .flags=0, .buf=td1316_init, .len=sizeof(td1316_init) };

	// setup PLL configuration
	if (i2c_transfer(&ru->adapter, &tuner_msg, 1) != 1) return -EIO;
	msleep(1);

	return 0;
}
/* ------------------------------------------------------------------------- */

static int philips_tu1216_pll_set(struct dvb_frontend* fe, struct dvb_frontend_parameters* params)
{
	struct reel_unit* ru = (struct reel_unit*) fe->dvb->priv;
	u8 tuner_buf[2];
	struct i2c_msg tuner_msg = {.addr=0x60, .flags=0, .buf=tuner_buf, .len=sizeof(tuner_buf) };
	int tuner_frequency = 0;
	int IF=36166666;
	u8 band, cp, filter;

	// determine charge pump
	tuner_frequency = params->frequency + IF;
	if (tuner_frequency < 87000000) return -EINVAL;
        else if (tuner_frequency < 130000000) cp = 3;
        else if (tuner_frequency < 160000000) cp = 5;
        else if (tuner_frequency < 200000000) cp = 6;
        else if (tuner_frequency < 290000000) cp = 3;
        else if (tuner_frequency < 420000000) cp = 5;
        else if (tuner_frequency < 480000000) cp = 6;
        else if (tuner_frequency < 620000000) cp = 3;
        else if (tuner_frequency < 830000000) cp = 5;
        else if (tuner_frequency < 895000000) cp = 6;
        else return -EINVAL;

	// determine band
	if (params->frequency < 49000000) return -EINVAL;
	else if (params->frequency < 161000000) band = 1;
	else if (params->frequency < 444000000) band = 2;
	else if (params->frequency < 861000000) band = 4;
	else return -EINVAL;

	// setup PLL filter
	switch (params->u.ofdm.bandwidth) {
	case BANDWIDTH_6_MHZ:
		tda1004x_write_byte(fe, 0x0C, 0);
		filter = 0;
		break;

	case BANDWIDTH_7_MHZ:
		tda1004x_write_byte(fe, 0x0C, 0);
		filter = 0;
		break;

	case BANDWIDTH_8_MHZ:
		tda1004x_write_byte(fe, 0x0C, 0xFF);
		filter = 1;
		break;

	default:
		return -EINVAL;
	}

	// GA: My PLL code
	tuner_frequency = (params->frequency+IF+1*83333)/166667;

	// Set Reference/CP
	tuner_buf[0] = CB1A_NO_ALBC; 
	tuner_buf[1] = (cp << 5) | (filter << 3) | band;

	if (i2c_transfer(&ru->adapter, &tuner_msg, 1) != 1)
		return -EIO;

	// setup tuner buffer
	printk("TDA: f %i\n",tuner_frequency);
	tuner_buf[0] = tuner_frequency >> 8;
	tuner_buf[1] = tuner_frequency & 0xff;

	if (i2c_transfer(&ru->adapter, &tuner_msg, 1) != 1)
		return -EIO;

	msleep(20);
	{
		u8 result[1]={0xff};
		struct i2c_msg tuner_msg1 = { .addr=0x60, .flags=I2C_M_RD, .buf=result, .len=1 };
		
		i2c_transfer(&ru->adapter, &tuner_msg1, 1);
		if (result[0]&0x20 == 0)
			printk("TD1316 PLL not locked!\n");
	
	}
	return 0;
}
/* ------------------------------------------------------------------------- */
static int philips_tu1216_request_firmware(struct dvb_frontend* fe, const struct firmware **fw, char* name)
{
	struct reel_unit* ru = (struct reel_unit*) fe->dvb->priv;

	return request_firmware(fw, name, &ru->rfpga->pci_dev->dev);
}
/* ------------------------------------------------------------------------- */
static int philips_tu1216_get_tune_settings(struct dvb_frontend *fe, 
					    struct dvb_frontend_tune_settings *fetunesettings)
{
	fetunesettings->step_size=500;
	fetunesettings->max_drift=500;
	fetunesettings->min_delay_ms = 5000;
	return 0;
}
/* ------------------------------------------------------------------------- */
static struct tda1004x_config philips_tu1216_config = {
	.demod_address = 0x8,
	.invert = 1,
	.invert_oclk = 0,
	.pll_init = philips_tu1216_pll_init,
	.pll_set = philips_tu1216_pll_set,
	.request_firmware = philips_tu1216_request_firmware,
};
