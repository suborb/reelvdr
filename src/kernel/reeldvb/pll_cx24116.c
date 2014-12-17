/*****************************************************************************/
/*
 * Settings for Sharp BS2F7VZ0165 with Conexant CX24116/24118
 * Used in DVB-S2 tuner module for the Reelbox
 *
 * (c) 2006 Georg Acher, BayCom GmbH (acher at baycom dot de)
 *
 * #include <gpl-header>
 *
 * $Id: $
 */
/*****************************************************************************/

#include "cx24116.h"

/* ------------------------------------------------------------------------- */
static int cx24116_get_tune_settings(struct dvb_frontend *fe, 
				     struct dvb_frontend_tune_settings *fetunesettings)
{
	fetunesettings->step_size=0;
	fetunesettings->max_drift=0;
	fetunesettings->min_delay_ms = 500; // For pilot auto tune
	return 0;
}
/* ------------------------------------------------------------------------- */
static int sharp_bs2f7_reset_sub(struct reel_unit* ru)
{
#if 1
        struct reelfpga *rfpga=ru->rfpga;
	int idreg;

	idreg=readl(rfpga->regbase+REEL_GPIO_IDOUT);

	if (ru->hw_unit==0 || ru->hw_unit==2) {
		// SLOT 1
		writel( (idreg | 1), rfpga->regbase+REEL_GPIO_IDOUT);
		msleep(1);
		writel( (idreg & ~1), rfpga->regbase+REEL_GPIO_IDOUT);
	}
	else {
		// SLOT 2
		writel( (idreg | 0x100), rfpga->regbase+REEL_GPIO_IDOUT);
		msleep(1);
		writel( (idreg & ~0x100), rfpga->regbase+REEL_GPIO_IDOUT);		
	}
#endif
	return 0;
}
/* ------------------------------------------------------------------------- */
static int sharp_bs2f7_reset(struct dvb_frontend *fe)
{
	struct reel_unit* ru = (struct reel_unit*) fe->dvb->priv;
	sharp_bs2f7_reset_sub(ru);
}
/* ------------------------------------------------------------------------- */
static struct cx24116_config sharp_bs2f7config = {	
	.demod_address = 0xa/2,
	.clock_polarity = 1,
	.reset_device = sharp_bs2f7_reset
	
};
