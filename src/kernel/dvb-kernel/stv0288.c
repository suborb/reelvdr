/*
  Driver for the ST STV0288 channel decoder

  (c) 2006 Georg Acher, BayCom GmbH, acher (at) baycom (dot) de
	  for Reel Multimedia

  Code parts are recycled from the somewhat similar stv0299.c

  #include <gpl_v2.h>

*/

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/string.h>
#include <linux/slab.h>
#include <asm/div64.h>

#include "dvb_frontend.h"
#include "stv0288.h"

unsigned char stv0288_inittab[]={
	// 0    1    2    3    4    5    6    7    8    9    a    b    c    d    e    f
	0x10,0x35,0x20,0x8e,0x8e,0x12,0x00,0x20,0x00,0x00,0x04,0x00,0x00,0x00,0xd4,0x40,   // 0
	0x80,0x7a,0x01,0x48,0x84,0x45,0xb8,0x91,0x00,0xa6,0x88,0x8f,0xf0,0x00,0x80,0x28,   // 1
	0x02,0xa6,0xff,0xd6,0x9a,0x7e,0x13,0x87,0x07,0xae,0x20,0xff,0xf1,0x1c,0x2b,0x08,   // 2
	0x00,0x1e,0x14,0x0f,0x09,0x05,0x05,0x2f,0x16,0xbc,0x13,0x13,0x11,0x30,0x00,0x00,   // 3
	0x63,0x04,0x60,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xdb,0x19,0x00,0x00,0x00,   // 4
	0x10,0x2e,0x09,0x83,0x38,0x05,0x5d,0x06,0x54,0x86,0x00,0x9b,0x08,0x7f,0x00,0xff,   // 5
	0x82,0x82,0x82,0x02,0x02,0x02,0x82,0x82,0x82,0x82,0x38,0x0c,0x00,0x00,0x00,0x00,   // 6
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,   // 7
	0x00,0x00,0x3f,0x3f,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,   // 8
	0x00,0x00,0x00,0x00,0x1c,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,   // 9
	0x48,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,   // a
	0xb8,0x3a,0x10,0x82,0x80,0x82,0x82,0x82,0x20,0x00,0x00,0x00,0x00,0x00,0x00,0x00    // b
};

static int debug;

#define dprintk(args...) \
        do { \
                if (debug) printk(KERN_DEBUG "stv0288: " args); \
        } while (0)

static int stv0288_writeregI (struct stv0288_state* state, u8 reg, u8 data)
{
        int ret;
        u8 buf [] = { reg, data };
        struct i2c_msg msg = { .addr = state->config->demod_address, .flags = 0, .buf = buf, .len = 2 };

        ret = i2c_transfer (state->i2c, &msg, 1);

        if (ret != 1)
                dprintk("%s: writereg error (reg == 0x%02x, val == 0x%02x, "
                        "ret == %i)\n", __FUNCTION__, reg, data, ret);

        return (ret != 1) ? -EREMOTEIO : 0;
}

int stv0288_writereg (struct dvb_frontend* fe, u8 reg, u8 data)
{
        struct stv0288_state* state = fe->demodulator_priv;

        return stv0288_writeregI(state, reg, data);
}

static u8 stv0288_readreg (struct stv0288_state* state, u8 reg)
{
        int ret;
        u8 b0 [] = { reg };
        u8 b1 [] = { 0 };
        struct i2c_msg msg [] = { { .addr = state->config->demod_address, .flags = 0, .buf = b0, .len = 1 },
                           { .addr = state->config->demod_address, .flags = I2C_M_RD, .buf = b1, .len = 1 } };

        ret = i2c_transfer (state->i2c, msg, 2);

        if (ret != 2)
                dprintk("%s: readreg error (reg == 0x%02x, ret == %i)\n",
                                __FUNCTION__, reg, ret);

        return b1[0];
}

static int stv0288_readregs (struct stv0288_state* state, u8 reg1, u8 *b, u8 len)
{
        int ret;
        struct i2c_msg msg [] = { { .addr = state->config->demod_address, .flags = 0, .buf = &reg1, .len = 1 },
                           { .addr = state->config->demod_address, .flags = I2C_M_RD, .buf = b, .len = len } };

        ret = i2c_transfer (state->i2c, msg, 2);

        if (ret != 2)
                dprintk("%s: readreg error (ret == %i)\n", __FUNCTION__, ret);

        return ret == 2 ? 0 : ret;
}

static int stv0288_wait_diseqc_fifo (struct stv0288_state* state, int timeout)
{
        unsigned long start = jiffies;

        dprintk ("%s\n", __FUNCTION__);

        while (stv0288_readreg(state, 0x07) & 0x40) {
                if (jiffies - start > timeout) {
                        dprintk ("%s: timeout!!\n", __FUNCTION__);
                        return -ETIMEDOUT;
                }
                msleep(1);
        };

        return 0;
}

static int stv0288_wait_diseqc_idle (struct stv0288_state* state, int timeout)
{
        unsigned long start = jiffies;

        dprintk ("%s\n", __FUNCTION__);

        while ((stv0288_readreg(state, 0x07) & 0x1f) != 0  ) {
                if (jiffies - start > timeout) {
                        dprintk ("%s: timeout!!\n", __FUNCTION__);
                        return -ETIMEDOUT;
                }
                msleep(10);
        }
        return 0;
}
static int stv0288_send_diseqc_msg (struct dvb_frontend* fe,
                                    struct dvb_diseqc_master_cmd *m)
{
        struct stv0288_state* state = fe->demodulator_priv;
        int i;

        dprintk ("%s\n", __FUNCTION__);

        if (stv0288_wait_diseqc_idle (state, 100) < 0)
                return -ETIMEDOUT;

        if (stv0288_writeregI (state, 0x05, 0x02))  /* DiSEqC mode */
                return -EREMOTEIO;

        for (i=0; i<m->msg_len; i++) {
                if (stv0288_wait_diseqc_fifo (state, 100) < 0)
                        return -ETIMEDOUT;

                if (stv0288_writeregI (state, 0x06, m->msg[i]))
                        return -EREMOTEIO;
        }

        if (stv0288_wait_diseqc_idle (state, 100) < 0)
                return -ETIMEDOUT;

        return 0;
}

static int stv0288_send_diseqc_burst (struct dvb_frontend* fe, fe_sec_mini_cmd_t burst)
{
        struct stv0288_state* state = fe->demodulator_priv;
        u8 val;

        dprintk ("%s %i\n", __FUNCTION__,burst);

        if (stv0288_wait_diseqc_idle (state, 100) < 0)
                return -ETIMEDOUT;

        val = stv0288_readreg (state, 0x05);

        if (stv0288_writeregI (state, 0x05, 3))        /* burst mode */ // FIXME
                return -EREMOTEIO;

        if (stv0288_writeregI (state, 0x06, burst == SEC_MINI_A ? 0x00 : 0xff))
                return -EREMOTEIO;

        if (stv0288_wait_diseqc_idle (state, 100) < 0)
                return -ETIMEDOUT;

        if (stv0288_writeregI (state, 0x05, val))
                return -EREMOTEIO;

        return 0;
}

static int stv0288_set_tone (struct dvb_frontend* fe, fe_sec_tone_mode_t tone)
{
        struct stv0288_state* state = fe->demodulator_priv;

        if (stv0288_wait_diseqc_idle (state, 100) < 0)
                return -ETIMEDOUT;

        switch (tone) {
        case SEC_TONE_ON:
                return stv0288_writeregI (state, 0x05,  0x0);

        case SEC_TONE_OFF:
                return stv0288_writeregI (state, 0x05,  0x02);

        default:
                return -EINVAL;
        }
}

static int stv0288_set_FEC (struct stv0288_state* state, fe_code_rate_t fec)
{
	switch (fec) {
	case FEC_AUTO:
		return stv0288_writeregI (state, 0x37, 0x2f);

	case FEC_1_2:
		return stv0288_writeregI (state, 0x37, 0x01);

	case FEC_2_3:
		return stv0288_writeregI (state, 0x37, 0x02);

	case FEC_3_4:
		return stv0288_writeregI (state, 0x37, 0x04);

	case FEC_5_6:
		return stv0288_writeregI (state, 0x37, 0x08);

	case FEC_7_8:
		return stv0288_writeregI (state, 0x37, 0x10);

	default:
		return -EINVAL;
	}	
}

static fe_code_rate_t stv0288_get_fec (struct stv0288_state* state)
{
	static fe_code_rate_t fec_tab [] = { FEC_1_2, FEC_2_3, FEC_3_4, FEC_5_6,
					     FEC_6_7, FEC_7_8 };
	u8 index;

	dprintk ("%s\n", __FUNCTION__);

	index = stv0288_readreg (state, 0x24);
	index &= 0x7;

	if (index > 5)
		return FEC_AUTO;

	return fec_tab [index];
}

static int stv0288_set_symbolrate (struct dvb_frontend* fe, u32 srate)
{
	struct stv0288_state* state = fe->demodulator_priv;
	u64 big = srate;

	if ((srate < 1000000) || (srate > 45000000)) return -EINVAL;

	big = big << 20;
	big += (state->config->mclk-1); // round correctly
	do_div(big, state->config->mclk);
	big=big<<4;
	
	stv0288_writeregI(state, 0x28, 0x80);
	stv0288_writeregI(state, 0x29, 0x00);
	stv0288_writeregI(state, 0x2a, 0x00);
	
	stv0288_writeregI(state, 0x28, (big>>16)&0xff);
	stv0288_writeregI(state, 0x29, (big>>8)&0xff);
	stv0288_writeregI(state, 0x2a, big&0xf0);	              	
	return 0;
}

static int stv0288_get_symbolrate (struct stv0288_state* state)
{
	u32 Mclk = state->config->mclk / 4096L;
	u32 srate;

	u8 sfr[3];
	s64 offset;
	u8 rtf[2];
	s16 rtfs;

	dprintk ("%s\n", __FUNCTION__);

	stv0288_readregs (state, 0x28, sfr, 3);
	stv0288_readregs (state, 0x22, rtf, 2);

	srate = (sfr[0] << 8) | sfr[1];
	srate *= Mclk;
	srate /= 16;
	srate += (sfr[2] >> 4) * Mclk / 256;
	rtfs= (rtf[0]<<8)|rtf[1];
	offset = rtfs;
	offset *= srate;
	offset = offset>>22; // datasheet says 19...

	dprintk ("%s : srate = %i offset %i\n", __FUNCTION__, srate,(int)offset);

	srate += offset;
	srate += 1000;
	srate /= 2000;
	srate *= 2000;

	return srate;
}

#if 0
static int stv0288_set_voltage (struct dvb_frontend* fe, fe_sec_voltage_t voltage)
{
}
#endif


static int stv0288_read_status(struct dvb_frontend* fe, fe_status_t* status)
{
        struct stv0288_state* state = fe->demodulator_priv;
	u8 signal = (stv0288_readreg (state, 0x10)+0x80)&0xff;
	u8 sync = stv0288_readreg (state, 0x24);

	dprintk ("%s : FE_READ_STATUS : VSTATUS: 0x%02x\n", __FUNCTION__, sync);
	*status = 0;

	if (signal > 30)
		*status |= FE_HAS_SIGNAL;

	if (sync & 0x80)
		*status |= FE_HAS_CARRIER;

	if (sync & 0x10)
		*status |= FE_HAS_VITERBI;

	if (sync & 0x08)
		*status |= FE_HAS_SYNC;

	if ((sync & 0x98) == 0x98)
		*status |= FE_HAS_LOCK;

	return 0;
}

static int stv0288_read_ber(struct dvb_frontend* fe, u32* ber)
{
        struct stv0288_state* state = fe->demodulator_priv;	
	// Uses counter 1, set to viterbi errors
	*ber=(stv0288_readreg(state,0x26)<<8)|stv0288_readreg(state,0x27);
	return 0;
}

static int stv0288_read_signal_strength(struct dvb_frontend* fe, u16* strength)
{
        struct stv0288_state* state = fe->demodulator_priv;
	u32 val;
	val=(stv0288_readreg(state, 0x10)+0x80)&0xff;
	val=val<<8 | val;
	val=(val*4)/3;
	if (val>0xffff)
		val=0xffff;
	*strength= val;
	return 0;
}

static int stv0288_read_snr(struct dvb_frontend* fe, u16* snr)
{
        struct stv0288_state* state = fe->demodulator_priv;

	*snr=0xffff-((stv0288_readreg(state,0x2d)<<8)|stv0288_readreg(state,0x2e));
	
	return 0;
}

static int stv0288_read_ucblocks(struct dvb_frontend* fe, u32* ucblocks)
{
        struct stv0288_state* state = fe->demodulator_priv;
	// Uses counter 2, set to packet errors
	*ucblocks=(stv0288_readreg(state,0x3e)<<8)|stv0288_readreg(state,0x3f);
	return 0;
}

static int stv0288_set_frontend(struct dvb_frontend* fe, struct dvb_frontend_parameters * p)
{
        struct stv0288_state* state = fe->demodulator_priv;
//        int invval = 0;

        dprintk ("%s : FE_SET_FRONTEND\n", __FUNCTION__);

        // set inversion not needed -> Auto Inversion
#if 0        
        if (state->config->enhanced_tuning) {
                /* check if we should do a finetune */
                int frequency_delta = p->frequency - state->tuner_frequency;
                int minmax = p->u.qpsk.symbol_rate / 2000;
                if (minmax < 5000) minmax = 5000;

                if ((frequency_delta > -minmax) && (frequency_delta < minmax) && (frequency_delta != 0) &&
                    (state->fec_inner == p->u.qpsk.fec_inner) &&
                    (state->symbol_rate == p->u.qpsk.symbol_rate)) {
                        int Drot_freq = (frequency_delta << 16) / (state->config->mclk / 1000);

                        // zap the derotator registers first
                        stv0288_writeregI(state, 0x2b, 0x00);
                        stv0288_writeregI(state, 0x2c, 0x00);

                        // now set them as we want
                        stv0288_writeregI(state, 0x2b, Drot_freq >> 8);
                        stv0288_writeregI(state, 0x2c, Drot_freq);
                } else {
                        /* A "normal" tune is requested */

                        state->config->pll_set(fe, p);

                        stv0288_writeregI(state, 0x38, 0x80);
                        stv0288_writeregI(state, 0x2b, 0x00);
                        stv0288_writeregI(state, 0x2c, 0x00);
                        stv0288_writeregI(state, 0x32, 0x16);
                        stv0288_set_symbolrate (fe, p->u.qpsk.symbol_rate);
                        stv0288_set_FEC (state, p->u.qpsk.fec_inner);
                }
        } else 
#endif
	{

                state->config->pll_set(fe, p);

                stv0288_set_FEC (state, p->u.qpsk.fec_inner);
                stv0288_set_symbolrate (fe, p->u.qpsk.symbol_rate);
                stv0288_writeregI(state, 0x22, 0x00);
                stv0288_writeregI(state, 0x23, 0x00);
                stv0288_writeregI(state, 0x2b, 0x00);
                stv0288_writeregI(state, 0x2c, 0x00);
		stv0288_writeregI(state, 0x15, 0xff);
        }

        state->tuner_frequency = p->frequency;
        state->fec_inner = p->u.qpsk.fec_inner;
        state->symbol_rate = p->u.qpsk.symbol_rate;

        return 0;
}

static int stv0288_get_frontend(struct dvb_frontend* fe, struct dvb_frontend_parameters * p)
{
        struct stv0288_state* state = fe->demodulator_priv;
        s32 derot_freq;

        derot_freq = (s32)(s16) ((stv0288_readreg (state, 0x2b) << 8)
                                | stv0288_readreg (state, 0x2c));

        derot_freq *= (state->config->mclk >> 16);
        derot_freq += 500;
        derot_freq /= 1000;

        p->frequency += derot_freq;

        p->inversion =  INVERSION_OFF;

        p->u.qpsk.fec_inner = stv0288_get_fec (state);
        p->u.qpsk.symbol_rate = stv0288_get_symbolrate (state);

        return 0;
}

/*static*/ int stv0288_sleep(struct dvb_frontend* fe)
{
        struct stv0288_state* state = fe->demodulator_priv;

        stv0288_writeregI(state, 0x41, 0x84);
        state->initialised = 0;

        return 0;
}

static int stv0288_get_tune_settings(struct dvb_frontend* fe, struct dvb_frontend_tune_settings* fesettings)
{
        struct stv0288_state* state = fe->demodulator_priv;

        fesettings->min_delay_ms = state->config->min_delay_ms;
        if (fesettings->parameters.u.qpsk.symbol_rate < 10000000) {
                fesettings->step_size = fesettings->parameters.u.qpsk.symbol_rate / 8000;
                fesettings->max_drift = 6000;
        } else {
                fesettings->step_size = fesettings->parameters.u.qpsk.symbol_rate / 8000;
                fesettings->max_drift = fesettings->parameters.u.qpsk.symbol_rate / 2000;
        }
        return 0;
}

static void stv0288_release(struct dvb_frontend* fe)
{
        struct stv0288_state* state = fe->demodulator_priv;
        kfree(state);
}

static int stv0288_init (struct dvb_frontend* fe)
{
        struct stv0288_state* state = fe->demodulator_priv;
        int i;

        printk("stv0288: init chip\n");
        stv0288_writeregI(state, 0x41, 0x4);
	msleep(50);
        for (i=0; i<sizeof(stv0288_inittab); i++)
                stv0288_writeregI(state, i, stv0288_inittab[i]);

	stv0288_writeregI(state, 0xf1, 0x00);
	stv0288_writeregI(state, 0xf0, 0x00);
	stv0288_writeregI(state, 0xf2, 0xe0);

        if (state->config->pll_init) {
                stv0288_writeregI(state, 0x01, 0xb5);   /*  enable i2c repeater on stv0288  */
                state->config->pll_init(fe);
                stv0288_writeregI(state, 0x01, 0x35);   /*  disable i2c repeater on stv0288  */
        }
	
        return 0;
}

static struct dvb_frontend_ops stv0288_ops;

struct dvb_frontend* stv0288_attach(const struct stv0288_config* config,
                                    struct i2c_adapter* i2c)
{
        struct stv0288_state* state = NULL;
        int id;

        /* allocate memory for the internal state */
        state = kmalloc(sizeof(struct stv0288_state), GFP_KERNEL);
        if (state == NULL) goto error;

        /* setup the state */
        state->config = config;
        state->i2c = i2c;
        memcpy(&state->ops, &stv0288_ops, sizeof(struct dvb_frontend_ops));
        state->initialised = 0;
        state->tuner_frequency = 0;
        state->symbol_rate = 0;
        state->fec_inner = 0;

        /* check if the demod is there */
        stv0288_writeregI(state, 0x41, 0x04); /* standby off */
        msleep(50);
        id = stv0288_readreg(state, 0x00);

        if ( (id&0xf0) != 0x10) goto error;

        stv0288_writeregI(state, 0x41, 0x84); /* standby on */
        /* create dvb_frontend */
        state->frontend.ops = &state->ops;
        state->frontend.demodulator_priv = state;
        return &state->frontend;

error:
        kfree(state);
        return NULL;
}

static struct dvb_frontend_ops stv0288_ops = {

        .info = {
                .name                   = "ST STV0288 DVB-S",
                .type                   = FE_QPSK,
                .frequency_min          = 950000,
                .frequency_max          = 2150000,
                .frequency_stepsize     = 125,   /* kHz for QPSK frontends */
                .frequency_tolerance    = 0,
                .symbol_rate_min        = 1000000,
                .symbol_rate_max        = 45000000,
                .symbol_rate_tolerance  = 500,  /* ppm */
                .caps = FE_CAN_FEC_1_2 | FE_CAN_FEC_2_3 | FE_CAN_FEC_3_4 |
                      FE_CAN_FEC_5_6 | FE_CAN_FEC_7_8 |
                      FE_CAN_QPSK |
                      FE_CAN_FEC_AUTO
        },

        .release = stv0288_release,

        .init = stv0288_init,
        .sleep = stv0288_sleep,

        .set_frontend = stv0288_set_frontend,
        .get_frontend = stv0288_get_frontend,
        .get_tune_settings = stv0288_get_tune_settings,

        .read_status = stv0288_read_status,
        .read_ber = stv0288_read_ber,
        .read_signal_strength = stv0288_read_signal_strength,
        .read_snr = stv0288_read_snr,
        .read_ucblocks = stv0288_read_ucblocks,

        .diseqc_send_master_cmd = stv0288_send_diseqc_msg,
        .diseqc_send_burst = stv0288_send_diseqc_burst,
        .set_tone = stv0288_set_tone,
//        .set_voltage = stv0288_set_voltage,
//        .dishnetwork_send_legacy_command = stv0288_send_legacy_dish_cmd,
};

module_param(debug, int, 0644);
MODULE_PARM_DESC(debug, "Turn on/off frontend debugging (default:off).");

MODULE_DESCRIPTION("ST STV0288 DVB Demodulator driver");
MODULE_AUTHOR("Georg Acher");
MODULE_LICENSE("GPL");

EXPORT_SYMBOL(stv0288_sleep);
EXPORT_SYMBOL(stv0288_writereg);
EXPORT_SYMBOL(stv0288_attach);
