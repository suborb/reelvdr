/*****************************************************************************/
/*
 * TUA6100-PLL driver for Philips SU1278 DVB-S NIM
 * 
 * (c) 2005 Georg Acher, BayCom GmbH (acher at baycom dot de)
 *
 * Extracted from old stv0299.c
 * (c) by
 * <ralph@convergence.de>, 
 * <holger@convergence.de>,
 * <js@convergence.de>,
 * Peter Schildmann <peter.schildmann@web.de>
 * Felix Domke <tmbinc@elitedvb.net>
 * Andreas Oberritter <obi@linuxtv.org>
 * Vadim Catana <skystar@moldova.cc>
 * Andrew de Quincey <adq_dvb@lidskialf.net>
 *
 * #include <gpl-header>
 *
 * $Id: pll_tua6100.c,v 1.1 2005/09/10 14:26:54 acher Exp $
 */
/*****************************************************************************/
#include "stv0299.h"

#define MIN2(a,b) ((a) < (b) ? (a) : (b))
#define MIN3(a,b,c) MIN2(MIN2(a,b),c)

/* ------------------------------------------------------------------------- */

static u8 philips_su1278_inittab [] = {
	0x01, 0x15,
	0x02, 0x00,   // TUA6100
	0x03, 0x00,
	0x04, 0x7d,   /* F22FR = 0x7d				      */
		      /* F22 = f_VCO / 128 / 0x7d = 22 kHz	      */

	/* I2C bus repeater */
	0x05, 0x35,   /* I2CT = 0, SCLT = 1, SDAT = 1		      */

	/* general purpose DAC registers */
	0x06, 0x40,   /* DAC not used, set to high impendance mode    */
	0x07, 0x00,   /* DAC LSB				      */

	/* DiSEqC registers */
	0x08, 0x40,   /* DiSEqC off, LNB power on OP2/LOCK pin on     */
	0x09, 0x00,   /* FIFO					      */

	/* Input/Output configuration register */
	0x0c, 0x51,   /* OP1 ctl = Normal, OP1 val = 1 (LNB Power ON) */
		      /* OP0 ctl = Normal, OP0 val = 1 (18 V)	      */
		      /* Nyquist filter = 00, QPSK reverse = 0	      */

	/* AGC1 control register */
	0x0d, 0x82,   /* DC offset compensation = ON, beta_agc1 = 2   */

	/* Timing loop register */
	0x0e, 0x23,   /* alpha_tmg = 2, beta_tmg = 3		      */

	0x0f, 0x94,   // TUA6100

	0x10, 0x3f,   // AGC2  0x3d

	0x11, 0x84,
	0x12, 0xb5,   // Lock detect: -64  Carrier freq detect:on

	0x15, 0xc9,   // lock detector threshold

	0x16, 0x00,
	0x17, 0x00,
	0x18, 0x00,
	0x19, 0x00,
	0x1a, 0x00,

	0x1f, 0x50,

	0x20, 0x00,
	0x21, 0x00,
	0x22, 0x00,
	0x23, 0x00,

	0x28, 0x00,  // out imp: normal	 out type: parallel FEC mode:0

	0x29, 0x1e,  // 1/2 threshold
	0x2a, 0x14,  // 2/3 threshold
	0x2b, 0x0f,  // 3/4 threshold
	0x2c, 0x09,  // 5/6 threshold
	0x2d, 0x05,  // 7/8 threshold
	0x2e, 0x01,

	0x31, 0x1f,  // test all FECs

	0x32, 0x10|0x08|0x01,  // viterbi and synchro search
	0x33, 0xfc,  // rs control
	0x34, 0x93,  // error control
	0xff, 0xff   // End
};

/* ------------------------------------------------------------------------- */

static int tua6100_pll_write (struct dvb_frontend* fe, u8 addr, u8 *data, int len)
{
      	struct reel_unit * ru = (struct reel_unit*) fe->dvb->priv;
	struct i2c_adapter *i2c = &ru->adapter;
	int ret;
        struct i2c_msg msg = { addr: addr, .flags = 0, .buf = data, .len = len };

	stv0299_writereg(fe, 0x05, 0xb5);
        ret =  i2c_transfer (i2c, &msg, 1);
	stv0299_writereg(fe, 0x05, 0x35);
        if (ret != 1)
                printk("%s: i/o error (ret == %i)\n", __FUNCTION__, ret);

        return (ret != 1) ? -1 : 0;
}
/* ------------------------------------------------------------------------- */
static int philips_su1278_pll_set(struct dvb_frontend* fe, struct dvb_frontend_parameters* params)
{
	struct reel_unit * ru = (struct reel_unit*) fe->dvb->priv;
	u8 reg0 [2] = { 0x00, 0x00 };
	u8 reg1 [4] = { 0x01, 0x00, 0x00, 0x00 };
	u8 reg2 [3] = { 0x02, 0x00, 0x00 };
	int _fband;
	int first_ZF;
	int R, A, N, P, M;
	int err;
	u32 freq,nfreq;
	u32 srate;

	freq=params->frequency;
	srate=params->u.qpsk.symbol_rate; 

	// GA: Powerdown PLL
	if (!freq) {
		u8 xreg0 [2] = { 0x00, 0x00 };
		u8 xreg1 [4] = { 0x01, 0xff, 0xff, 0xff };
		u8 xreg2 [3] = { 0x02, 0xff, 0xff };
		down(&ru->fe_mutex);
		tua6100_pll_write(fe, 0x60, xreg0, sizeof(xreg0));
		tua6100_pll_write(fe, 0x60, xreg1, sizeof(xreg1));
		tua6100_pll_write(fe, 0x60, xreg2, sizeof(xreg2));	        
		up(&ru->fe_mutex);
		ru->freq=freq;
    	        return 0;
	}

	first_ZF = (freq) / 1000;

	if (abs(MIN2(abs(first_ZF-1190),abs(first_ZF-1790))) <
	    abs(MIN3(abs(first_ZF-1202),abs(first_ZF-1542),abs(first_ZF-1890))))
		_fband = 2;
	else
		_fband = 3;

	if (_fband == 2) {
		if (((first_ZF >= 950) && (first_ZF < 1350)) ||
		    ((first_ZF >= 1430) && (first_ZF < 1950)))
			reg0[1] = 0x07;
		else if (((first_ZF >= 1350) && (first_ZF < 1430)) ||
			    ((first_ZF >= 1950) && (first_ZF < 2150)))
			reg0[1] = 0x0B;
	}

	if(_fband == 3) {
		if (((first_ZF >= 950) && (first_ZF < 1350)) ||
		     ((first_ZF >= 1455) && (first_ZF < 1950)))
			reg0[1] = 0x07;
		else if (((first_ZF >= 1350) && (first_ZF < 1420)) ||
			 ((first_ZF >= 1950) && (first_ZF < 2150)))
			reg0[1] = 0x0B;
		else if ((first_ZF >= 1420) && (first_ZF < 1455))
			reg0[1] = 0x0F;
	}

	if (first_ZF > 1525)
		reg1[1] |= 0x80;
	else
		reg1[1] &= 0x7F;

	if (_fband == 2) {
		if (first_ZF > 1430) { /* 1430MHZ */
			reg1[1] &= 0xCF; /* N2 */
			reg2[1] &= 0xCF; /* R2 */
			reg2[1] |= 0x10;
		} else {
			reg1[1] &= 0xCF; /* N2 */
			reg1[1] |= 0x20;
			reg2[1] &= 0xCF; /* R2 */
			reg2[1] |= 0x10;
		}
	}

	if (_fband == 3) {
		if ((first_ZF >= 1455) &&
		    (first_ZF < 1630)) {
			reg1[1] &= 0xCF; /* N2 */
			reg1[1] |= 0x20;
			reg2[1] &= 0xCF; /* R2 */
		} else {
			if (first_ZF < 1455) {
				reg1[1] &= 0xCF; /* N2 */
				reg1[1] |= 0x20;
				reg2[1] &= 0xCF; /* R2 */
				reg2[1] |= 0x10;
			} else {
				if (first_ZF >= 1630) {
					reg1[1] &= 0xCF; /* N2 */
					reg2[1] &= 0xCF; /* R2 */
					reg2[1] |= 0x10;
				}
			}
		}
	}

	/* GA: set ports, enable P0 for symbol rates > 8Ms/s 
	   SU1278 states >4Ms/s, but looking at the filter output
	   its better with >8Ms/s.
	*/
	if (srate >= 2*4000000)
		reg1[1] |= 0x0c;
	else
		reg1[1] |= 0x08;

	reg2[1] |= 0x0c;
#if 0
	R = 64;
	A = 0;
	P = 64;	 //32

	M = (freq * R) / 4;		/* in Mhz */
	N = (M - A * 1000) / (P * 1000);
#else
	// GA: above formula is quite crappy and
	// gives wrong values...
	nfreq=freq/125;
	R=64;
	P=64;
	A=(2*(nfreq))&(R-1);  // 1/16MHz Steps
	N=nfreq/(4*8);  // 64*1/16=4 MHz Steps

#endif
	printk("N %i, A %i, R %i, f %i rf %i\n",N,A,R,freq,(((P*N)+A)*4000)/R);

	reg1[1] |= (N >> 9) & 0x03;
	reg1[2]	 = (N >> 1) & 0xff;
	reg1[3]	 = (N << 7) & 0x80;

	reg2[1] |= (R >> 8) & 0x03;
	reg2[2]	 = R & 0xFF;	/* R */

	reg1[3] |= A & 0x7f;	/* A */

	if (P == 64)
		reg1[1] |= 0x40; /* Prescaler 64/65 */

	reg0[1] |= 0x03;

	down(&ru->fe_mutex);
	
        if (1) {
	//if (abs(ru->freq-freq)>2000) {
		// GA: Kick PLL to avoid hanging VCO
		u8 xreg0 [2] = { 0x00, 0x0f };
		u8 xreg1 [4] = { 0x01, 0xff, 0xff, 0x00 };
		u8 xreg2 [3] = { 0x02, 0xff, 0xff };
		if (srate<8000000)
			xreg1[1]&=~4;
		tua6100_pll_write(fe, 0x60, xreg0, sizeof(xreg0));
		tua6100_pll_write(fe, 0x60, xreg1, sizeof(xreg1));
		tua6100_pll_write(fe, 0x60, xreg2, sizeof(xreg2));
	}
	err=0;
	
	if ((err = tua6100_pll_write(fe, 0x60, reg2, sizeof(reg2))))
		goto error;

	if ((err = tua6100_pll_write(fe, 0x60, reg0, sizeof(reg0))))
		goto error;

	if ((err = tua6100_pll_write(fe, 0x60, reg1, sizeof(reg1))))
		goto error;
	ru->freq=freq;
error:
	up(&ru->fe_mutex);
	return err;
}
/* ------------------------------------------------------------------------- */

static int philips_su1278_set_symbol_rate(struct dvb_frontend *fe, u32 srate, u32 ratio)
{
        u8 aclk = 0;
        u8 bclk = 0;
        u8 m1;

        aclk = 0xb5;
        if (srate < 2000000)
                bclk = 0x86;
        else if (srate < 5000000)
                bclk = 0x89;
        else if (srate < 15000000)
                bclk = 0x8f;
        else if (srate < 45000000)
                bclk = 0x95;

        m1 = 0x14;
        if (srate < 4000000)
                m1 = 0x10;

        stv0299_writereg(fe, 0x13, aclk);
        stv0299_writereg(fe, 0x14, bclk);
        stv0299_writereg(fe, 0x1f, (ratio >> 16) & 0xff);
        stv0299_writereg(fe, 0x20, (ratio >> 8) & 0xff);
        stv0299_writereg(fe, 0x21, (ratio) & 0xf0);
        stv0299_writereg(fe, 0x0f, 0x80 | m1);

        return 0;
}
/* ------------------------------------------------------------------------- */
static int philips_su1278_sleep(struct dvb_frontend *fe)
{
	struct dvb_frontend_parameters params;
	struct stv0299_state* state = fe->demodulator_priv;

	reel_set_dvbs_voltage(fe, SEC_VOLTAGE_OFF);
	params.frequency=0;
	philips_su1278_pll_set(fe, &params);
	stv0299_writereg(fe, 0x02, 0x80);
	state->initialised = 0;
	return 0;
}
/* ------------------------------------------------------------------------- */
static int philips_su1278_get_tune_settings(struct dvb_frontend *fe, 
					    struct dvb_frontend_tune_settings *fetunesettings)
{
	fetunesettings->step_size=125;
	fetunesettings->max_drift=1500;
	fetunesettings->min_delay_ms = 100;
	return 0;
}
/* ------------------------------------------------------------------------- */

static struct stv0299_config philips_su1278_config = {	
	.demod_address = 0x68,
	.inittab = philips_su1278_inittab,
	.mclk = 88000000UL,
	.invert = 0,
	.enhanced_tuning = 0,
	.skip_reinit = 0,
	.min_delay_ms = 100,
	.set_symbol_rate = philips_su1278_set_symbol_rate,
	.pll_set = philips_su1278_pll_set
};

/* ------------------------------------------------------------------------- */
