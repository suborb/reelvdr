/*****************************************************************************/
/*
 * Frontend drivers for Reelbox (4 tuner version)
 *
 * Copyright (C) 2004-2007  Georg Acher, BayCom GmbH (acher at baycom dot de)
 *
 * #include <GPL-V2-header>
 *
 *  $Id: reel_frontend.c,v 1.4 2006/01/29 22:23:11 acher Exp $
 *
 */       
/*****************************************************************************/


#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/smp_lock.h>
#include <linux/vmalloc.h>
#include <asm/io.h>
#include <linux/pci.h>
#include <linux/wait.h>
#include <linux/delay.h>

#include "reeldvb.h"

#define dbg(format, arg...) printk(format"\n", ## arg)
#define err(format, arg...) printk(format"\n", ## arg)

// For SU1278 (DVB-S)
#include "pll_tua6100.c"

// For DNBU10x12/IRT/IRB (DVB-S)
#include "pll_stb6000.c"

// For CU1216 (DVB_C)
#include "pll_tda6509.c"

// For TU1216 (DVB-T)
#include "pll_tda6651.c"

#define HAS_CX24116
#ifdef HAS_CX24116
// For CX24116/8 (DVB-S2)
#include "pll_cx24116.c"
#endif

/* --------------------------------------------------------------------- */
// Own handler for LNB power control

int reel_set_dvbs_voltage(struct dvb_frontend* fe, fe_sec_voltage_t voltage) 
{
	struct reel_unit *ru = fe->dvb->priv;
	int value=0;
	int ret;
	
	if (ru->lnb_voltage<voltage)  // Don't disturb disecq
		msleep(20);
	
	switch(voltage) {
	case SEC_VOLTAGE_OFF:
		value=0;
		break;
	case SEC_VOLTAGE_13:
		value|=0x14;
		break;
	case SEC_VOLTAGE_18:
		value|=0xc;
		break;
	default:
		return -EINVAL;
	}

	ret=reel_lnbp21_write(ru, 0x80|value);	// Run with static current protection

	if (ru->lnb_voltage==0)
		msleep(100);

	if (ru->lnb_voltage<voltage)
		msleep(20);

	msleep(5);

	if (ru->nim_id==REEL_NIM_TWIN_DNBU || ru->nim_id==REEL_NIM_TWIN_DNBUV2)
		value|=0x40;

	ret=reel_lnbp21_write(ru,value);	// Switch to dynamic current protection

	ru->lnb_voltage=voltage;

	return ret;
}
/* --------------------------------------------------------------------- */
// Initialise frontend based on NIM ID

void reel_frontend_init(struct reel_unit *ru)
{	
	ru->dvb_frontend=NULL;

	switch(ru->nim_id) {
	case REEL_NIM_SU1278:
		printk("Attach DVB-S Philips SU1278 (STV0299/TUA6100) to unit %i\n", ru->num);
		ru->dvb_frontend = stv0299_attach(&philips_su1278_config, &ru->adapter);
		ru->algo_data.udelay=3; // Speed up I2C 
		if (ru->dvb_frontend) {
			ru->dvb_frontend->ops->set_voltage = reel_set_dvbs_voltage;
			ru->dvb_frontend->ops->sleep = philips_su1278_sleep;
//			ru->dvb_frontend->ops->get_tune_settings = philips_su1278_get_tune_settings;
		}
		break;

	case REEL_NIM_CU1216:
		printk("Attach DVB-C Philips CU1216(TDA10023/TDA6651) to unit %i\n", ru->num);
		ru->dvb_frontend = tda10023_attach(&philips_cu1216_config, &ru->adapter,cu1216_read_pwm);
		break;

	case REEL_NIM_TU1216:
		printk("Attach DVB-T Philips TU1216(TDA10046/TDA6509) to unit %i\n", ru->num);
		ru->dvb_frontend = tda10046_attach(&philips_tu1216_config, &ru->adapter);
		ru->algo_data.udelay=3; // Speed up I2C (100kHz)
		ru->dvb_frontend->ops->get_tune_settings = philips_tu1216_get_tune_settings;
		break;

	case REEL_NIM_TWIN_DNBU:
		printk("Attach DVB-S Samsung DNBU10x12IRB (STV0299/STB6000) to unit %i\n", ru->num);
		ru->dvb_frontend = stv0299_attach(&samsung_dnbu_config, &ru->adapter);
		if (ru->dvb_frontend) {
			ru->dvb_frontend->ops->set_voltage = reel_set_dvbs_voltage;
			ru->dvb_frontend->ops->sleep = samsung_dnbu_sleep;
		}
		break;
		
	case REEL_NIM_TWIN_DNBUV2:

		printk("Attach DVB-S Samsung DNBU10x12IRT (STV0288/STB6000) to unit %i\n", ru->num);
		ru->dvb_frontend = stv0288_attach(&samsung_dnbuv2_config, &ru->adapter);
		ru->algo_data.udelay=3; // Speed up I2C 
		if (ru->dvb_frontend) {
			ru->dvb_frontend->ops->set_voltage = reel_set_dvbs_voltage;
			ru->dvb_frontend->ops->sleep = samsung_dnbuv2_sleep;
		}
		break;
#ifdef HAS_CX24116
	case REEL_NIM_DVBS2:
		printk("Attach DVB-S2 to unit %i\n", ru->num);
		sharp_bs2f7_reset_sub(ru); // Kick it for reliable detection
		ru->dvb_frontend = cx24116_attach(&sharp_bs2f7config, &ru->adapter);
		ru->algo_data.udelay=1; // Speed up I2C 
		if (ru->dvb_frontend) {
			ru->dvb_frontend->ops->set_voltage = reel_set_dvbs_voltage;
			ru->dvb_frontend->ops->get_tune_settings = cx24116_get_tune_settings;
		}
#endif
		break;
		// More to come...

	case REEL_NIM_NONE:
		printk("No Tuner for unit %i\n", ru->num);
		break;

	default:	
		break;
	}

	if (!ru->dvb_frontend) {
		if (ru->nim_id!=REEL_NIM_NONE)
			err("reel_frontend_init: Unsupported/unrecognized frontend id %i for unit %i\n", ru->nim_id, ru->num);
	}
	else {
		if (dvb_register_frontend(&ru->dvb_adapter, ru->dvb_frontend)) {
			printk("reel_frontend_init: Frontend registration failed for unit %i!\n",ru->num);
			if (ru->dvb_frontend->ops->release)
				ru->dvb_frontend->ops->release(ru->dvb_frontend);
			ru->dvb_frontend = NULL;
		}
	}
}


