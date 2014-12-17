/*****************************************************************************/
/*
 * STB6000-PLL driver for Samsung DNBU10x12 DVB-S NIM
 * Used in Twin-Tuner-Module for Reelbox 
 * 
 * The STB6000 is a zero-IF, direct conversion downmixer with fully
 * integrated PLL. Except the loop filter, no external VCO components
 * (varicaps, ...) or IF-filters (like in the SU1278) are needed.
 *
 * But don't expect a reduced power consumption ;-)
 * 
 * (c) 2005 Georg Acher, BayCom GmbH (acher at baycom dot de)
 *
 * #include <gpl-header>
 *
 * $Id: pll_stb6000.c,v 1.2 2005/09/10 14:34:16 acher Exp $
 */
/*****************************************************************************/

#include "stv0299.h"
#include "stv0288.h"

#define STB6000_ADDR (0xc0/2)

/* ------------------------------------------------------------------------- */
static int stv0299_pll_write (struct dvb_frontend* fe, u8 addr, u8 reg, u8 val)
{
      	struct reel_unit * ru = (struct reel_unit*) fe->dvb->priv;
	struct i2c_adapter *i2c = &ru->adapter;
	u8 data[2]={reg,val};
        struct i2c_msg msg = { addr: addr, .flags = 0, .buf = data, .len = 2 };
	int ret;

	stv0299_writereg(fe, 0x05, 0xb5);
        ret =  i2c_transfer (i2c, &msg, 1);
	stv0299_writereg(fe, 0x05, 0x35);
        if (ret != 1)
                printk("%s: i/o error (ret == %i)\n", __FUNCTION__, ret);
	
        return (ret != 1) ? -1 : 0;
}
/* ------------------------------------------------------------------------- */
static int stv0288_pll_write (struct dvb_frontend* fe, u8 addr, u8 reg, u8 val)
{
      	struct reel_unit * ru = (struct reel_unit*) fe->dvb->priv;
	struct i2c_adapter *i2c = &ru->adapter;
	u8 data[2]={reg,val};
        struct i2c_msg msg = { addr: addr, .flags = 0, .buf = data, .len = 2 };
	int ret;

	stv0288_writereg(fe, 0x01, 0xb5);
        ret =  i2c_transfer (i2c, &msg, 1);
	stv0288_writereg(fe, 0x01, 0x35);
        if (ret != 1)
                printk("%s: i/o error (ret == %i)\n", __FUNCTION__, ret);
	
        return (ret != 1) ? -1 : 0;
}
/* ------------------------------------------------------------------------- */

static int samsung_pll_set(struct dvb_frontend* fe, struct dvb_frontend_parameters* params, 
			   int (*pll_write)(struct dvb_frontend* fe,u8 addr, u8 reg, u8 val))
{
	struct reel_unit * ru = (struct reel_unit*) fe->dvb->priv;
	u8 reg[8];
	int a,m,n,odiv,r,p,osm,k;
	u32 freq,freq1,fref;
	int srate;

	freq1=params->frequency;
	freq=params->frequency/1000;

	if (!freq) {		
		down(&ru->fe_mutex);
		pll_write(fe, STB6000_ADDR, 10, 0);     // PLL disable
		up(&ru->fe_mutex);
    	        return 0;
	}

	// Calculate baseband filter setting depending on symbolrate
	srate=params->u.qpsk.symbol_rate;
	srate=srate/1000000;
	srate=srate-3;
	if (srate<0)
		srate=0;
	if (srate>31)
		srate=31;

	// The wonders of fully integrated VCOs...
	if (freq<=999)
		osm=10;
	else if (freq<=1075)
		osm=12;
	else if (freq<=1199)
		osm=0;
	else if (freq<=1299)
		osm=1;
	else if (freq<=1369)
		osm=2;
	else if (freq<=1469)
		osm=4;
	else if (freq<=1529)
		osm=5;
	else if (freq<=1649)
		osm=6;
	else if (freq<=1799)
		osm=8;
	else if (freq<=1949)
		osm=10;
	else 
		osm=12;

	if (freq>=1076) {
		odiv=0;
		m=2;
	}
	else {
		odiv=1;
		m=4;
		freq1=freq1*2;
	}

	k = 0;                // Xtal div /1
	r = 8;
	p = 16;	
	fref = 4; 

	a = ((2*freq1)/1000)&15;                     // a stepsize fref/r   = 0.25MHz
	n = (r*((freq1/1000)&~(fref*p/r-1)))/(fref*p);  // n stepsize fref*p/r = 8MHz

	reg[1]=0xa0 | 0x10*odiv | osm;
	reg[2]=n>>1;
	reg[3]=((n&1)<<7) | a;
	reg[4]=(k<<6) | r;
	reg[5]=7;           // Baseband Gain 0dB
	reg[6]=0x80+srate;  // LPF bandwidth
	reg[7]=0xd8;        // DLB=160Hz, FCL=/1

//	printk("Needed Freq %i, a %i, n %i, m %i, result %i, srate %i\n",freq1/(m/2), a,n,m,(((2*fref*1000)/m)*(p*n+a))/r,srate);

	pll_write(fe, STB6000_ADDR, 11, 0x4f);  // 4MHz
	pll_write(fe, STB6000_ADDR, 10, 0x00);  // PLL disable
	pll_write(fe, STB6000_ADDR, 1, reg[1]);
	pll_write(fe, STB6000_ADDR, 2, reg[2]);
	pll_write(fe, STB6000_ADDR, 3, reg[3]);
	pll_write(fe, STB6000_ADDR, 4, reg[4]);
	pll_write(fe, STB6000_ADDR, 5, reg[5]);
	pll_write(fe, STB6000_ADDR, 7, reg[7]);
	pll_write(fe, STB6000_ADDR, 6, reg[6]);
	pll_write(fe, STB6000_ADDR, 7, reg[7]|7); // FCL disable
	pll_write(fe, STB6000_ADDR, 10, 0xfb);    // PLL enable
#if 1
	// HACK: Because the readback of the PLL lock flag doesn't work for some reason,
	// we assume that it has a lock after 20ms.
	msleep(20);
	pll_write(fe, STB6000_ADDR, 1, (reg[1]&0x1f)|0x60); // Turn off VCO search
#endif
	return 0;
}
/* ------------------------------------------------------------------------- */
static int samsung_dnbu_pll_set(struct dvb_frontend* fe, struct dvb_frontend_parameters* params)
{
	return samsung_pll_set(fe,params,stv0299_pll_write);
}
/* ------------------------------------------------------------------------- */
static int samsung_dnbuv2_pll_set(struct dvb_frontend* fe, struct dvb_frontend_parameters* params)
{
	return samsung_pll_set(fe,params,stv0288_pll_write);
}
/* ------------------------------------------------------------------------- */
static int samsung_dnbu_sleep(struct dvb_frontend *fe)
{
	struct dvb_frontend_parameters params;

	reel_set_dvbs_voltage(fe, SEC_VOLTAGE_OFF);
	params.frequency=0;
	samsung_dnbu_pll_set(fe, &params);
	stv0299_sleep(fe);

	// FIXME Shutdown analog 5V via reel_gpio_idreg()
	return 0;
}
/* ------------------------------------------------------------------------- */
static int samsung_dnbuv2_sleep(struct dvb_frontend *fe)
{
	struct dvb_frontend_parameters params;

	reel_set_dvbs_voltage(fe, SEC_VOLTAGE_OFF);
	params.frequency=0;
	samsung_dnbuv2_pll_set(fe, &params);
	stv0288_sleep(fe);

	// FIXME Shutdown analog 5V via reel_gpio_idreg()
	return 0;
}
/* ------------------------------------------------------------------------- */

static struct stv0299_config samsung_dnbu_config = {	
	.demod_address = 0x68,
	.inittab = philips_su1278_inittab,
	.mclk = 88000000UL,
	.invert = 0,
	.enhanced_tuning = 0,
	.skip_reinit = 0,
	.min_delay_ms = 100,
	.set_symbol_rate = philips_su1278_set_symbol_rate,
	.pll_set = samsung_dnbu_pll_set,
};

static struct stv0288_config samsung_dnbuv2_config = {	
	.demod_address = 0x68,
	.mclk = 100000000UL,
	.invert = 0,
//	.enhanced_tuning = 0,
//	.skip_reinit = 0,
	.min_delay_ms = 100,
	.pll_set = samsung_dnbuv2_pll_set,
};
