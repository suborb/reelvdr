/*
    Conexant cx24116/cx24118 - DVBS/S2 Satellite demod/tuner driver

    Copyright (C) 2006 Steven Toth <stoth@hauppauge.com>
    Copyright (C) 2006 Georg Acher (acher (at) baycom (dot) de) for Reel Multimedia
                       Added Diseqc, auto pilot tuning and hack for old DVB-API

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#include <linux/slab.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/firmware.h>


#include "dvb_frontend.h"
#include "dvb_new_frontend.h"
#include "cx24116.h"

#define CX24116_DEFAULT_FIRMWARE "dvb-fe-cx24116.fw"
#define CX24116_SEARCH_RANGE_KHZ 5000

static int debug = 0;
#define dprintk(args...) \
	do { \
		if (debug) printk ("cx24116: " args); \
	} while (0)

enum cmds
{
	CMD_SET_VCO     = 0x10,
	CMD_TUNEREQUEST = 0x11,
	CMD_MPEGCONFIG  = 0x13,
	CMD_TUNERINIT   = 0x14,
	CMD_BANDWIDTH   = 0x15,
	CMD_GETAGC      = 0x19,
	CMD_LNBCONFIG   = 0x20,
	CMD_LNBSEND     = 0x21,
	CMD_SET_TONEPRE = 0x22,
	CMD_SET_TONE    = 0x23,
	CMD_UPDFWVERS   = 0x35,
	CMD_TUNERSLEEP  = 0x36,
	CMD_AGCCONTROL  = 0x3b,
	CMD_MAX         = 0xFF
};

struct firmware_cmd
{
	enum cmds id;	/* Unique firmware command */
	int len;	/* Commands args len + id byte */
} CX24116_COMMANDS[] =
{
	{ CMD_SET_VCO,     0x0A },
	{ CMD_TUNEREQUEST, 0x13 },
	{ CMD_MPEGCONFIG,  0x06 },
	{ CMD_TUNERINIT,   0x03 },
	{ CMD_BANDWIDTH,   0x02 },
	{ CMD_GETAGC,   0x01 },
	{ CMD_LNBCONFIG,   0x08 }, 
	{ CMD_LNBSEND,     0x0c },
	{ CMD_SET_TONEPRE, 0x02 },
	{ CMD_SET_TONE,    0x04 },
	{ CMD_UPDFWVERS,   0x02 },
	{ CMD_TUNERSLEEP,  0x02 },
	{ CMD_AGCCONTROL,  0x04 },
	{ CMD_MAX,         0x00 }
};

/* The Demod/Tuner can't easily provide these, we cache them */
struct cx24116_tuning
{
	u32 frequency;
	u32 symbol_rate;
	fe_spectral_inversion_t inversion;
	enum dvbfe_fec fec;

	enum dvbfe_delsys delivery;
	enum dvbfe_rolloff rolloff;

	int pilot; // 0: off, 1: on (only used for S2)

	/* Demod values */
	u8 fec_val;
	u8 fec_mask;
	u8 inversion_val;
};

/* Basic commands that are sent to the firmware */
struct cx24116_cmd
{
	enum cmds id;
	u8 len;
	u8 args[0x1e];
};

struct cx24116_state
{
	struct i2c_adapter* i2c;
	const struct cx24116_config* config;
	struct dvb_frontend_ops ops;

	struct dvb_frontend frontend;

	struct cx24116_tuning dcur;
	struct cx24116_tuning dnxt;

	u8 skip_fw_load;
};

static int cx24116_writereg(struct cx24116_state* state, int reg, int data)
{
	u8 buf[] = { reg, data };
	struct i2c_msg msg = { .addr = state->config->demod_address,
		.flags = 0, .buf = buf, .len = 2 };
	int err;

	if (debug>1)
		printk("cx24116: %s:  write reg 0x%02x, value 0x%02x\n",
						__FUNCTION__,reg, data);

	if ((err = i2c_transfer(state->i2c, &msg, 1)) != 1) {
		printk("%s: writereg error(err == %i, reg == 0x%02x,"
			 " data == 0x%02x)\n", __FUNCTION__, err, reg, data);
		return -EREMOTEIO;
	}

	return 0;
}

/* Bulk byte writes to a single I2C address, for 32k firmware load */
static int cx24116_writeregN(struct cx24116_state* state, int reg, u8 *data, u16 len)
{
	int ret = -EREMOTEIO;
	struct i2c_msg msg;
	u8 *buf;

	buf = kmalloc(len + 1, GFP_KERNEL);
	if (buf == NULL) {
		printk("Unable to kmalloc\n");
		ret = -ENOMEM;
		goto error;
	}

	*(buf) = reg;
	memcpy(buf + 1, data, len);

       	msg.addr = state->config->demod_address;
	msg.flags = 0;
	msg.buf = buf;
	msg.len = len + 1;

	if (debug>1)
		printk("cx24116: %s:  write regN 0x%02x, len = %d\n",
						__FUNCTION__,reg, len);

	if ((ret = i2c_transfer(state->i2c, &msg, 1)) != 1) {
		printk("%s: writereg error(err == %i, reg == 0x%02x\n",
			 __FUNCTION__, ret, reg);
		ret = -EREMOTEIO;
	}

error:
	kfree(buf);

	return ret;
}

static int cx24116_readreg(struct cx24116_state* state, u8 reg)
{
	int ret;
	u8 b0[] = { reg };
	u8 b1[] = { 0 };
	struct i2c_msg msg[] = {
		{ .addr = state->config->demod_address, .flags = 0, .buf = b0, .len = 1 },
		{ .addr = state->config->demod_address, .flags = I2C_M_RD, .buf = b1, .len = 1 }
	};

	ret = i2c_transfer(state->i2c, msg, 2);

	if (ret != 2) {
		printk("%s: reg=0x%x (error=%d)\n", __FUNCTION__, reg, ret);
		return ret;
	}

	if (debug>1)
		printk("cx24116: read reg 0x%02x, value 0x%02x\n",reg, ret);

	return b1[0];
}

/* GA Hack: Inversion is nwo implicitely auto. The set_inversion-call is used via the 
   zig zag scan to find the correct the pilot on/off-modes for S2 tuning.  
   The demod can't detect it on its own :-(
*/

static int cx24116_set_inversion(struct cx24116_state* state, fe_spectral_inversion_t inversion)
{
	dprintk("%s(%d)\n", __FUNCTION__, inversion);

	if (inversion==INVERSION_OFF)
		state->dnxt.pilot=0;
	else
		state->dnxt.pilot=1;

	state->dnxt.inversion =  INVERSION_AUTO;
	state->dnxt.inversion_val = 0x0C; // Off 0x00, On: 0x04

	return 0;
}

static int cx24116_get_inversion(struct cx24116_state* state, fe_spectral_inversion_t *inversion)
{
	dprintk("%s()\n", __FUNCTION__);
	*inversion = state->dcur.inversion;
	return 0;
}

/* A table of modulation, fec and configuration bytes for the demod.
 * Not all S2 mmodulation schemes are support and not all rates with
 * a scheme are support. Especially, no auto detect when in S2 mode.
 */
struct cx24116_modfec {
	enum dvbfe_delsys delsys;
	enum dvbfe_modulation modulation;
	enum dvbfe_fec fec;
	u8 mask;	/* In DVBS mode this is used to autodetect */
	u8 val;		/* Passed to the firmware to indicate mode selection */
} CX24116_MODFEC_MODES[] = {
 /* QPSK. For unknown rates we set to hardware to to detect 0xfe 0x30 */
 { DVBFE_DELSYS_DVBS,  DVBFE_MOD_QPSK, DVBFE_FEC_NONE, 0xfe, 0x30 },
 { DVBFE_DELSYS_DVBS,  DVBFE_MOD_QPSK, DVBFE_FEC_1_2,  0x02, 0x2e },
 { DVBFE_DELSYS_DVBS,  DVBFE_MOD_QPSK, DVBFE_FEC_2_3,  0x04, 0x2f },
 { DVBFE_DELSYS_DVBS,  DVBFE_MOD_QPSK, DVBFE_FEC_3_4,  0x08, 0x30 },
 { DVBFE_DELSYS_DVBS,  DVBFE_MOD_QPSK, DVBFE_FEC_4_5,  0xfe, 0x30 },
 { DVBFE_DELSYS_DVBS,  DVBFE_MOD_QPSK, DVBFE_FEC_5_6,  0x20, 0x31 },
 { DVBFE_DELSYS_DVBS,  DVBFE_MOD_QPSK, DVBFE_FEC_6_7,  0xfe, 0x30 },
 { DVBFE_DELSYS_DVBS,  DVBFE_MOD_QPSK, DVBFE_FEC_7_8,  0x80, 0x32 },
 { DVBFE_DELSYS_DVBS,  DVBFE_MOD_QPSK, DVBFE_FEC_8_9,  0xfe, 0x30 },
 { DVBFE_DELSYS_DVBS,  DVBFE_MOD_QPSK, DVBFE_FEC_AUTO, 0xfe, 0x30 },
 /* NBC-QPSK */
 { DVBFE_DELSYS_DVBS2, DVBFE_MOD_QPSK, DVBFE_FEC_1_2,  0x00, 0x04 },
 { DVBFE_DELSYS_DVBS2, DVBFE_MOD_QPSK, DVBFE_FEC_3_5,  0x00, 0x05 },
 { DVBFE_DELSYS_DVBS2, DVBFE_MOD_QPSK, DVBFE_FEC_2_3,  0x00, 0x06 },
 { DVBFE_DELSYS_DVBS2, DVBFE_MOD_QPSK, DVBFE_FEC_3_4,  0x00, 0x07 },
 { DVBFE_DELSYS_DVBS2, DVBFE_MOD_QPSK, DVBFE_FEC_4_5,  0x00, 0x08 },
 { DVBFE_DELSYS_DVBS2, DVBFE_MOD_QPSK, DVBFE_FEC_5_6,  0x00, 0x09 },
 { DVBFE_DELSYS_DVBS2, DVBFE_MOD_QPSK, DVBFE_FEC_8_9,  0x00, 0x0a },
 { DVBFE_DELSYS_DVBS2, DVBFE_MOD_QPSK, DVBFE_FEC_9_10, 0x00, 0x0b },
 /* 8PSK */
 { DVBFE_DELSYS_DVBS2, DVBFE_MOD_8PSK, DVBFE_FEC_3_5,  0x00, 0x0c},
 { DVBFE_DELSYS_DVBS2, DVBFE_MOD_8PSK, DVBFE_FEC_2_3,  0x00, 0x0d },
 { DVBFE_DELSYS_DVBS2, DVBFE_MOD_8PSK, DVBFE_FEC_3_4,  0x00, 0x0e },
 { DVBFE_DELSYS_DVBS2, DVBFE_MOD_8PSK, DVBFE_FEC_5_6,  0x00, 0x0f },
 { DVBFE_DELSYS_DVBS2, DVBFE_MOD_8PSK, DVBFE_FEC_8_9,  0x00, 0x10 },
 { DVBFE_DELSYS_DVBS2, DVBFE_MOD_8PSK, DVBFE_FEC_9_10, 0x00, 0x11 },
};

static int cx24116_lookup_fecmod(struct cx24116_state* state,
	enum dvbfe_delsys d, enum dvbfe_modulation m, enum dvbfe_fec f)
{
	unsigned int i;
	int ret = -1;

	for(i=0 ; i < sizeof(CX24116_MODFEC_MODES) / sizeof(struct cx24116_modfec) ; i++)
	{
		if( (d == CX24116_MODFEC_MODES[i].delsys) &&
			(m == CX24116_MODFEC_MODES[i].modulation) &&
			(f == CX24116_MODFEC_MODES[i].fec) )
			{
				ret = i;
				break;
			}
	}

	return ret;
}

static int cx24116_set_fec(struct cx24116_state* state, struct dvbfe_params *p)
{
	int ret = -1;
	dprintk("%s()\n", __FUNCTION__);

	switch(p->delivery)
	{
		case DVBFE_DELSYS_DVBS:
			ret = cx24116_lookup_fecmod(state,
				p->delivery, p->delsys.dvbs.modulation, p->delsys.dvbs.fec);
			break;
		case DVBFE_DELSYS_DVBS2:
			ret = cx24116_lookup_fecmod(state,
				p->delivery, p->delsys.dvbs2.modulation, p->delsys.dvbs2.fec);
			break;
		default:
			ret = -ENOTSUPP;
	}
	if (ret>=0) {
		state->dnxt.fec_val = CX24116_MODFEC_MODES[ret].val;
		state->dnxt.fec_mask = CX24116_MODFEC_MODES[ret].mask;
		dprintk("%s() fec/mask = 0x%02x/0x%02x\n", __FUNCTION__,
			state->dnxt.fec, state->dnxt.fec_mask);
		ret=0;
	}
	return ret;
}

static int cx24116_get_fec_dvbs2(struct cx24116_state* state,
				  enum dvbfe_fec *fec)
{
	dprintk("%s()\n", __FUNCTION__);
	*fec = state->dcur.fec;
	return 0;
}

static int cx24116_get_fec_dvbs(struct cx24116_state* state,
				  enum dvbfe_fec *fec)
{
	int fectab[8]={FEC_NONE,FEC_1_2,FEC_2_3,FEC_3_4,FEC_4_5,FEC_5_6,FEC_6_7,FEC_7_8};
	u8 fecreg;
	dprintk("%s()\n", __FUNCTION__);

	fecreg=cx24116_readreg(state,0x0c)&7;
	*fec = fectab[fecreg];
	return 0;
}

static int cx24116_set_symbolrate(struct cx24116_state* state, struct dvbfe_params *p)
{
	int ret = 0;

	dprintk("%s()\n", __FUNCTION__);

	switch(p->delivery)
	{
		case DVBFE_DELSYS_DVBS:
			state->dnxt.symbol_rate = p->delsys.dvbs.symbol_rate;
			break;
		case DVBFE_DELSYS_DVBS2:
			state->dnxt.symbol_rate = p->delsys.dvbs2.symbol_rate;
			break;
		default:
			ret = -ENOTSUPP;
	}

	dprintk("%s() symbol_rate = %d\n", __FUNCTION__, state->dnxt.symbol_rate);
#if 0
	// FIXME
	/*  check if symbol rate is within limits */
	if ((state->dnxt.symbol_rate > state->frontend.ops.info.symbol_rate_max) ||
	    (state->dnxt.symbol_rate < state->frontend.ops.info.symbol_rate_min))
		ret = -EOPNOTSUPP;
#endif
	return ret;
}

static int cx24116_load_firmware (struct dvb_frontend* fe, const struct firmware *fw);

int cxinit=0;

static int cx24116_firmware_ondemand(struct dvb_frontend* fe)
{
	struct cx24116_state *state = fe->demodulator_priv;
	const struct firmware *fw;
	int ret = 0;
	int syschipid,gotreset;
	dprintk("%s()\n",__FUNCTION__);
	gotreset=cx24116_readreg(state, 0x20);  // implicit watchdog
	syschipid=cx24116_readreg(state, 0x94);

//	if (!cxinit)
	if (gotreset || syschipid!=5 )
	{
		if (state->skip_fw_load)
			return 0;

		/* Load firmware */
		/* request the firmware, this will block until someone uploads it */
		printk("%s: Waiting for firmware upload (%s)...\n", __FUNCTION__, CX24116_DEFAULT_FIRMWARE);
		ret = request_firmware(&fw, CX24116_DEFAULT_FIRMWARE, &state->i2c->dev);
		printk("%s: Waiting for firmware upload(2)...\n", __FUNCTION__);
		if (ret) {
			printk("%s: No firmware uploaded (timeout or file not found?)\n", __FUNCTION__);
			return ret;
		}

		/* Make sure we don't recurse back through here during loading */
		state->skip_fw_load = 1;

		ret = cx24116_load_firmware(fe, fw);
		if (ret)
			printk("%s: Writing firmware to device failed\n", __FUNCTION__);

		release_firmware(fw);

		printk("%s: Firmware upload %s\n", __FUNCTION__, ret == 0 ? "complete" : "failed");

		/* Ensure firmware is always loaded if required */
		state->skip_fw_load = 0;
		cxinit=1;
	}

	return ret;
}

/* Take a basic firmware command structure, format it and forward it for processing */
static int cx24116_cmd_execute(struct dvb_frontend* fe, struct cx24116_cmd *cmd)
{
	struct cx24116_state *state = fe->demodulator_priv;
	unsigned int i;
	int ret;

	/* dprintk("%s()\n", __FUNCTION__); */ //TB: less debug-output

	/* Load the firmware if required */
	if ( (ret = cx24116_firmware_ondemand(fe)) != 0)
	{
		printk("%s(): Unable initialise the firmware\n", __FUNCTION__);
		return ret;
	}

	for(i = 0; i < (sizeof(CX24116_COMMANDS) / sizeof(struct firmware_cmd)); i++ )
	{
		if(CX24116_COMMANDS[i].id == CMD_MAX)
			return -EINVAL;

		if(CX24116_COMMANDS[i].id == cmd->id)
		{
			cmd->len = CX24116_COMMANDS[i].len;
			break;
		}
	}

	cmd->args[0x00] = cmd->id;

	/* Write the command */
	for(i = 0; i < 0x1f/*cmd->len*/ ; i++)
	{
		if (i<cmd->len) {
			dprintk("%s: 0x%02x == 0x%02x\n", __FUNCTION__, i, cmd->args[i]);
			cx24116_writereg(state, i, cmd->args[i]);
		}
		else 
			cx24116_writereg(state, i, 0);
	}
	

	/* Start execution and wait for cmd to terminate */
	cx24116_writereg(state, 0x1f, 0x01);
	while( cx24116_readreg(state, 0x1f) )
	{
		msleep(10);
		if(i++ > 64)
		{
			/* Avoid looping forever if the firmware does no respond */
			printk("%s() Firmware not responding\n", __FUNCTION__);
			return -EREMOTEIO;
		}
	}
	return 0;
}

static int cx24116_load_firmware (struct dvb_frontend* fe, const struct firmware *fw)
{
	struct cx24116_state* state = fe->demodulator_priv;
	struct cx24116_cmd cmd;
	int ret,n,len;
	unsigned char fw_vers[4];

	dprintk("%s\n", __FUNCTION__);
	printk("Firmware is %zu bytes (%02x %02x .. %02x %02x)\n"
			,fw->size
			,fw->data[0]
			,fw->data[1]
			,fw->data[ fw->size-2 ]
			,fw->data[ fw->size-1 ]
			);

	/* Toggle 88x SRST pin to reset demod */
	if (state->config->reset_device)
		state->config->reset_device(fe);

	// PLL
	cx24116_writereg(state, 0xE5, 0x00);
	cx24116_writereg(state, 0xF1, 0x08);
	cx24116_writereg(state, 0xF2, 0x13);
	
	// Kick PLL
	cx24116_writereg(state, 0xe0, 0x03);
	cx24116_writereg(state, 0xe0, 0x00);

	/* Begin the firmware load process */
	/* Prepare the demod, load the firmware, cleanup after load */

	cx24116_writereg(state, 0xF3, 0x46);
	cx24116_writereg(state, 0xF9, 0x00);

	cx24116_writereg(state, 0xF0, 0x03);
	cx24116_writereg(state, 0xF4, 0x81);
	cx24116_writereg(state, 0xF5, 0x00);
	cx24116_writereg(state, 0xF6, 0x00);

#if 1
	/* write the entire firmware as one transaction */
	cx24116_writeregN(state, 0xF7, fw->data, fw->size);
#else
	/* Write in small blocks */
	len=fw->size;
        n=0;
#define MAX_TSIZE 2048
        do {
                cx24116_writeregN(state, 0xF7, fw->data+n, (len>MAX_TSIZE?MAX_TSIZE:len));
                n+=MAX_TSIZE;
                len-=MAX_TSIZE;
        } while(len>0);
#endif 

	cx24116_writereg(state, 0xF4, 0x10);
	cx24116_writereg(state, 0xF0, 0x00);
	cx24116_writereg(state, 0xF8, 0x06);

	/* Firmware CMD 10: Main init */
	cmd.id = CMD_SET_VCO;
	cmd.args[0x01] = 0x05;
	cmd.args[0x02] = 0xdc;
	cmd.args[0x03] = 0xda;
	cmd.args[0x04] = 0xae;
	cmd.args[0x05] = 0xaa;
	cmd.args[0x06] = 0x04;
	cmd.args[0x07] = 0x9d;
	cmd.args[0x08] = 0xfc;
	cmd.args[0x09] = 0x06;
	ret = cx24116_cmd_execute(fe, &cmd);
	if (ret != 0)
		return ret;

	cx24116_writereg(state, 0x9d, 0x00);

	/* Firmware CMD 14: Tuner Init */
	cmd.id = CMD_TUNERINIT;
	cmd.args[0x01] = 0x00;
	cmd.args[0x02] = 0x00;
	ret = cx24116_cmd_execute(fe, &cmd);
	if (ret != 0)
		return ret;

	cx24116_writereg(state, 0xe5, 0x00);

	/* Firmware CMD 13: MPEG/TS output config */
	cmd.id = CMD_MPEGCONFIG;
	cmd.args[0x01] = 0x01;
	cmd.args[0x02] = 0x75;
	cmd.args[0x03] = 0x00;
	cmd.args[0x04] = 0x02 + (state->config->clock_polarity&1);
	cmd.args[0x05] = 0x00;
	ret = cx24116_cmd_execute(fe, &cmd);
	if (ret != 0)
		return ret;

	// Firmware CMD 20: LNB/Diseqc Config

	cmd.id = CMD_LNBCONFIG;
	cmd.args[1]=0;
	cmd.args[2]=0x10;
	cmd.args[3]=0x00;
	cmd.args[4]=0x8f;
	cmd.args[5]=0x28;
	cmd.args[6]=0x00;	// Disable tone burst. Temporarily enabled later
	cmd.args[7]=0x01;

	ret = cx24116_cmd_execute(fe, &cmd);
	if (ret != 0)
		return ret;



	// Get firmware version
	for(n=0;n<4;n++) {
		cmd.id = CMD_UPDFWVERS;
		cmd.args[0x01] = n;
		ret = cx24116_cmd_execute(fe, &cmd);
		if (ret != 0)
			return ret;
		fw_vers[n]=cx24116_readreg(state,0x96);
	}
	printk("cx24116: FW version %i.%i.%i.%i\n",fw_vers[0],fw_vers[1],fw_vers[2],fw_vers[3]);
	return 0;
}

static int cx24116_initfe(struct dvb_frontend* fe)
{
	struct cx24116_state *state = fe->demodulator_priv;
	struct cx24116_cmd cmd;

	dprintk("%s()\n",__FUNCTION__);

	// Powerup
	cx24116_writereg(state, 0xe0, 0);
	cx24116_writereg(state, 0xe1, 0);
	cx24116_writereg(state, 0xea, 0);
	
	cmd.id = CMD_TUNERSLEEP;
	cmd.args[1]=0;
	cx24116_cmd_execute(fe, &cmd);

	return 0;
}

static int cx24116_set_voltage(struct dvb_frontend* fe, fe_sec_voltage_t voltage)
{
	/* The isl6421 module will override this function in the fops. */
	dprintk("%s() This should never appear if the isl6421 module is loaded correctly\n",__FUNCTION__);
	return -ENOTSUPP;
}

static int cx24116_read_status(struct dvb_frontend* fe, fe_status_t* status)
{
	struct cx24116_state *state = fe->demodulator_priv;

	int lock = cx24116_readreg(state, 0x9d);

//	dprintk("%s: status = 0x%02x\n", __FUNCTION__, lock);

	*status = 0;

	if (lock & 0x01)
		*status |= FE_HAS_SIGNAL;
	if (lock & 0x02)
		*status |= FE_HAS_CARRIER;
	if (lock & 0x04)
		*status |= FE_HAS_VITERBI;
	if (lock & 0x08)
		*status |= FE_HAS_SYNC | FE_HAS_LOCK;

	return 0;
}

static int cx24116_read_ber(struct dvb_frontend* fe, u32* ber)
{
	struct cx24116_state *state = fe->demodulator_priv;
	uint8_t ber3=cx24116_readreg(state,0xc6);
	uint8_t ber2=cx24116_readreg(state,0xc7);
	uint8_t ber1=cx24116_readreg(state,0xc8);
	uint8_t ber0=cx24116_readreg(state,0xc9);
//	dprintk("%s()\n", __FUNCTION__);
	
	*ber = (ber0<<24)|(ber1<<16)|(ber2<<8)| ber3; // FIXME S/S2

	return 0;
}

static int cx24116_read_signal_strength(struct dvb_frontend* fe, u16* signal_strength)
{
	struct cx24116_state *state = fe->demodulator_priv;
	u8 lsb,msb;
	int strength;
	struct cx24116_cmd cmd;

	// FW 1.23.x needs an undocumented spell
	cmd.id = CMD_GETAGC;
	cx24116_cmd_execute(fe, &cmd); // ignore retval for older FWs
	
	lsb=cx24116_readreg(state, 0x9e);
	msb=(cx24116_readreg(state, 0x9d))>>6;
	
	// Magic for useful scaling, cutting the noise floor a bit
	strength = (0x3ff-((msb<<8) | lsb))<<7;
	strength = strength - 0x5000;

	if (strength<0 )
		strength=0;
	if (strength>0xffff)
		strength=0xffff;
	*signal_strength = (u16)strength;

//	dprintk("%s:  Signal strength = %d\n",__FUNCTION__,*signal_strength);

	return 0;
}

static int cx24116_read_snr(struct dvb_frontend* fe, u16* snr)
{
	struct cx24116_state *state = fe->demodulator_priv;
	u8 lsb=cx24116_readreg(state,0xd5);
	u8 msb=cx24116_readreg(state,0xa3);
	int snrx;
//	dprintk("%s()\n", __FUNCTION__);

	snrx= (msb<<8) | lsb;	// Units: 0.1dB
	// To roughly match with other cards, scale 0-b0 to 0-ffff

	snrx=snrx*372;
	if (snrx>0xffff)
		snrx=0xffff;

	*snr = snrx;

	return 0;
}

static int cx24116_read_ucblocks(struct dvb_frontend* fe, u32* ucblocks)
{
	struct cx24116_state *state = fe->demodulator_priv;
	uint8_t msb=cx24116_readreg(state,0xca);
	uint8_t lsb=cx24116_readreg(state,0xcb);
//	dprintk("%s()\n", __FUNCTION__);
	*ucblocks = (msb<<8)|lsb;

	return 0;
}

/* Overwrite the current tuning params, we are about to tune */
static void cx24116_clone_params(struct dvb_frontend* fe)
{
	struct cx24116_state *state = fe->demodulator_priv;
	memcpy(&state->dcur, &state->dnxt, sizeof(state->dcur));
}

/* Why isn't this a generic frontend core function? */
static enum dvbfe_fec cx24116_convert_oldfec_to_new(enum fe_code_rate c)
{
	enum dvbfe_fec fec = DVBFE_FEC_NONE;

	switch(c)
	{
		case FEC_NONE: fec = DVBFE_FEC_NONE; break;
		case FEC_1_2 : fec = DVBFE_FEC_1_2 ; break;
		case FEC_2_3 : fec = DVBFE_FEC_2_3 ; break;
		case FEC_3_4 : fec = DVBFE_FEC_3_4 ; break;
		case FEC_4_5 : fec = DVBFE_FEC_4_5 ; break;
		case FEC_5_6 : fec = DVBFE_FEC_5_6 ; break;
		case FEC_6_7 : fec = DVBFE_FEC_6_7 ; break;
		case FEC_7_8 : fec = DVBFE_FEC_7_8 ; break;
		case FEC_8_9 : fec = DVBFE_FEC_8_9 ; break;
		case FEC_AUTO: fec = DVBFE_FEC_AUTO; break;
		case FEC_1_4 : fec = DVBFE_FEC_1_4 ; break; // S2 HACK
		case FEC_1_3 : fec = DVBFE_FEC_1_3 ; break; // S2
		case FEC_2_5 : fec = DVBFE_FEC_2_5 ; break; // S2
		case FEC_3_5 : fec = DVBFE_FEC_3_5 ; break; // S2
		case FEC_9_10 : fec = DVBFE_FEC_9_10; break; // S2
	}

	return fec;
}

/* Why isn't this a generic frontend core function? */
static enum dvbfe_fec cx24116_convert_newfec_to_old(enum dvbfe_fec c)
{
	fe_code_rate_t fec;

	switch(c)
	{
		case DVBFE_FEC_NONE: fec = FEC_NONE; break;
		case DVBFE_FEC_1_2 : fec = FEC_1_2 ; break;
		case DVBFE_FEC_2_3 : fec = FEC_2_3 ; break;
		case DVBFE_FEC_3_4 : fec = FEC_3_4 ; break;
		case DVBFE_FEC_4_5 : fec = FEC_4_5 ; break;
		case DVBFE_FEC_5_6 : fec = FEC_5_6 ; break;
		case DVBFE_FEC_6_7 : fec = FEC_6_7 ; break;
		case DVBFE_FEC_7_8 : fec = FEC_7_8 ; break;
		case DVBFE_FEC_8_9 : fec = FEC_8_9 ; break;
	        case DVBFE_FEC_1_4 : fec = FEC_1_4 ; break; // S2
	        case DVBFE_FEC_1_3 : fec = FEC_1_3 ; break; // S2
	        case DVBFE_FEC_2_5 : fec = FEC_2_5 ; break; // S2
	        case DVBFE_FEC_3_5 : fec = FEC_3_5 ; break; // S2
	        case DVBFE_FEC_9_10 : fec = FEC_9_10 ; break; // S2
		case DVBFE_FEC_AUTO: fec = FEC_AUTO; break;
		default: fec = FEC_NONE; break;
	}

	return fec;
}

/* Why isn't this a generic frontend core function? */
static int cx24116_create_new_qpsk_feparams(struct dvb_frontend* fe,
	struct dvb_frontend_parameters *pFrom,
	struct dvbfe_params *pTo)
{
	int ret = 0;
	int mod,rolloff;
	enum dvbfe_rolloff Rollofftab[4]={DVBFE_ROLLOFF_35, DVBFE_ROLLOFF_25, DVBFE_ROLLOFF_20, DVBFE_ROLLOFF_UNKNOWN};

	dprintk("%s\n", __FUNCTION__);

	memset(pTo, 0, sizeof(struct dvbfe_params));

	pTo->frequency = pFrom->frequency;
	pTo->inversion = pFrom->inversion;
	mod = (pFrom->u.qpsk.fec_inner>>16)&255;       // Hack to allow S2 modes in old DVB-API
							// modulation in bits 23-16s
	rolloff = (pFrom->u.qpsk.fec_inner>>24)&255;   // default is 0 -> 0.35
							// rolloff in bits 31-24

	if (mod==QAM_AUTO)  // CX24116 doesn't support auto for S2 modes -> backward compatibility
		mod=QPSK;

	pTo->delsys.dvbs.modulation = DVBFE_MOD_QPSK;
	
	if (mod!=QPSK || (pFrom->u.qpsk.fec_inner&255)>FEC_AUTO) {
		pTo->delivery = DVBFE_DELSYS_DVBS2;
		if (mod==PSK8)
			pTo->delsys.dvbs.modulation = DVBFE_MOD_8PSK;
		pTo->delsys.dvbs.rolloff = Rollofftab[rolloff&3];
	}
	else {
		pTo->delsys.dvbs.rolloff = DVBFE_ROLLOFF_35;
		pTo->delivery = DVBFE_DELSYS_DVBS;		
	}
	
	pTo->delsys.dvbs.symbol_rate = pFrom->u.qpsk.symbol_rate;
	pTo->delsys.dvbs.fec = cx24116_convert_oldfec_to_new(pFrom->u.qpsk.fec_inner&255);
	printk("cx24116: FREQ %i SR %i FEC %x FECN %08x MOD %i DS %i \n",
	       pTo->frequency, pTo->delsys.dvbs.symbol_rate, 
	       pFrom->u.qpsk.fec_inner,
	       pTo->delsys.dvbs.fec, pTo->delsys.dvbs.modulation,
	       pTo->delivery);
	/* I guess we could do more validation on the old fe params and return error */
	return ret;
}

/* Why isn't this a generic frontend core function? */
static int cx24116_create_old_qpsk_feparams(struct dvb_frontend* fe,
	struct dvbfe_params *pFrom,
	struct dvb_frontend_parameters *pTo)
{
	int ret = 0;

	dprintk("%s\n", __FUNCTION__);

	memset(pTo, 0, sizeof(struct dvb_frontend_parameters));

	pTo->frequency = pFrom->frequency;
	pTo->inversion = pFrom->inversion;

	switch(pFrom->delivery)
	{
		case DVBFE_DELSYS_DVBS:
			pTo->u.qpsk.fec_inner = cx24116_convert_newfec_to_old(pFrom->delsys.dvbs.fec);
			pTo->u.qpsk.symbol_rate = pFrom->delsys.dvbs.symbol_rate;
			break;
		default:
			ret = -1;
	}

	/* I guess we could do more validation on the old fe params and return error */
	return ret;
}

static int cx24116_wait_for_diseqc(struct cx24116_state* state)
{
	int n;
	for(n=0;n<50;n++) {
		if (cx24116_readreg(state,0xbc)&0x20)
			return 0;
		msleep(10);
	}
	printk("%s: Timeout\n", __FUNCTION__);
	return -1;
}

static int cx24116_send_diseqc_msg (struct dvb_frontend* fe,
                                    struct dvb_diseqc_master_cmd *m)
{
	struct cx24116_state* state = fe->demodulator_priv;
	struct cx24116_cmd cmd;
	int len= m->msg_len;
	int n,pos=0;
	int ret;

	dprintk("%s: length %i\n", __FUNCTION__, len);

	if (cx24116_wait_for_diseqc(state))
		return -ETIMEDOUT;


	cmd.id = CMD_LNBCONFIG;
	cmd.args[1]=0;
	cmd.args[2]=0x10;
	cmd.args[3]=0x00;
	cmd.args[4]=0x8f;
	cmd.args[5]=0x28;
	cmd.args[6]=0x00;	// disable tone burst
	cmd.args[7]=0x01;
	ret = cx24116_cmd_execute(fe, &cmd);
	if (ret != 0)
		return ret;

	cmd.id=CMD_LNBSEND;
	cmd.args[1]=0;
	cmd.args[2]=2;
	cmd.args[3]=0;
	cmd.args[4]=0;
	cmd.args[5]=4;
	
	while(len>0) {

		if (len>6) {
			cmd.args[4]=1; // Long message
			cmd.args[5]=6;
		}
		else {
			cmd.args[4]=0;
			cmd.args[5]=len;
		}

		for(n=0;n<6; n++)
			cmd.args[6+n]=m->msg[pos+n];

		ret = cx24116_cmd_execute(fe, &cmd);
		if (ret != 0)
			return ret;
		pos+=6;
		len=len-6;
	}

	return 0;
}

// FIXME: Check if tone burst really works...
static int cx24116_send_diseqc_burst (struct dvb_frontend* fe, fe_sec_mini_cmd_t burst)
{
	struct cx24116_state* state = fe->demodulator_priv;
	struct cx24116_cmd cmd;
	int ret;
	dprintk("%s: burst %i\n", __FUNCTION__,burst);
//	return;
	
	if (cx24116_wait_for_diseqc(state))
		return -ETIMEDOUT;

	cmd.id = CMD_LNBCONFIG;
	cmd.args[1]=0;
	cmd.args[2]=0x10;
	cmd.args[3]=0x00;
	cmd.args[4]=0x8f;
	cmd.args[5]=0x28;
	cmd.args[6]=0x01; // enable burst
	cmd.args[7]=0x01;

	ret = cx24116_cmd_execute(fe, &cmd);

	if (ret != 0)
		return ret;

	/* GA Hack: It's not possible to send a tone burst on its own
		without a DiSeqC-message. So either we need to wait
		for a real message, store it and transmit it with a
		tone burst (depends on tuning client, IMO not
		reliable, but stored for cx24116_send_diseqc_msg) or
		send a dummy message with all bits zero. So the
		framing nibble (0xE*) is missing and hopefully all
		devices ignore it.
	*/
		
       	cmd.id=CMD_LNBSEND;
	cmd.args[1]= (burst == SEC_MINI_A ? 0x00 : 0x01);
	cmd.args[2]=2;
	cmd.args[3]=0;
	cmd.args[4]=0;
	cmd.args[5]=3;     // Dummy 3 bytes
	cmd.args[6] = 0;
	cmd.args[7] = 0;
	cmd.args[8] = 0;

	ret = cx24116_cmd_execute(fe, &cmd);

	return ret;
}

static int cx24116_set_tone(struct dvb_frontend* fe, fe_sec_tone_mode_t tone)
{
	struct cx24116_cmd cmd;
	int ret;

	dprintk("%s(%d)\n", __FUNCTION__, tone);
	if ( (tone != SEC_TONE_ON) && (tone != SEC_TONE_OFF) ) {
		printk("%s: Invalid, tone=%d\n", __FUNCTION__, tone);
		return -EINVAL;
	}
	/* wait for diseqc queue ready */
	cx24116_wait_for_diseqc(fe->demodulator_priv);

	/* This is always done before the tone is set */
	cmd.id = CMD_SET_TONEPRE;
	cmd.args[0x01] = 0x00;
	ret = cx24116_cmd_execute(fe, &cmd);
	if (ret != 0)
		return ret;

	/* Now we set the tone */
	cmd.id = CMD_SET_TONE;
	cmd.args[0x01] = 0x00;
	cmd.args[0x02] = 0x00;

	switch (tone) {
	case SEC_TONE_ON:
		dprintk("%s: setting tone on\n", __FUNCTION__);
		cmd.args[0x03] = 0x01;
		break;
	case SEC_TONE_OFF:
		dprintk("%s: setting tone off\n",__FUNCTION__);
		cmd.args[0x03] = 0x00;
		break;
	}

	return cx24116_cmd_execute(fe, &cmd);
}

static int cx24116_sleep(struct dvb_frontend* fe)
{
	struct cx24116_state* state = fe->demodulator_priv;
	struct cx24116_cmd cmd;
	printk("%s\n",__FUNCTION__);
	cmd.id = CMD_TUNERSLEEP;
	cmd.args[1]=1;
	cx24116_cmd_execute(fe, &cmd);

	// Shutdown clocks
	cx24116_writereg(state, 0xea, 0xff);
	cx24116_writereg(state, 0xe1, 1);
	cx24116_writereg(state, 0xe0, 1);
	return 0;
}

static void cx24116_release(struct dvb_frontend* fe)
{
	struct cx24116_state* state = fe->demodulator_priv;
	dprintk("%s\n",__FUNCTION__);
	kfree(state);
}

static struct dvb_frontend_ops cx24116_ops;

struct dvb_frontend* cx24116_attach(const struct cx24116_config* config,
				    struct i2c_adapter* i2c)
{
	struct cx24116_state* state = NULL;
	int ret;

	dprintk("%s\n",__FUNCTION__);

	/* allocate memory for the internal state */
	state = kmalloc(sizeof(struct cx24116_state), GFP_KERNEL);
	if (state == NULL) {
		printk("Unable to kmalloc\n");
		goto error;
	}

	/* setup the state */
	memset(state, 0, sizeof(struct cx24116_state));

	state->config = config;
	state->i2c = i2c;

	/* check if the demod is present */
	ret = (cx24116_readreg(state, 0xFF) << 8) | cx24116_readreg(state, 0xFE);
	if (ret != 0x0501) {
		printk("Invalid probe, probably not a CX24116 device\n");
		goto error;
	}

	/* create dvb_frontend */
	memcpy(&state->ops, &cx24116_ops, sizeof(struct dvb_frontend_ops));
	state->frontend.ops = &state->ops;
	state->frontend.demodulator_priv = state;
	return &state->frontend;

error:
	kfree(state);

	return NULL;
}
#if 0
static struct dvbfe_info dvbs_info	= {
	.name				= "Conexant CX24116 DVB-S",
	.delivery			= DVBFE_DELSYS_DVBS,
	.delsys				= {
		.dvbs.modulation	= DVBFE_MOD_QPSK,
		.dvbs.fec		= DVBFE_FEC_1_2 | DVBFE_FEC_2_3 |
					  DVBFE_FEC_3_4 | DVBFE_FEC_4_5 |
					  DVBFE_FEC_5_6 | DVBFE_FEC_6_7 |
					  DVBFE_FEC_7_8 | DVBFE_FEC_AUTO
	},

	.frequency_min			= 950000,
	.frequency_max			= 2150000,
	.frequency_step			= 1011,
	.frequency_tolerance		= 5000,
	.symbol_rate_min		= 1000000,
	.symbol_rate_max		= 45000000
};

static const struct dvbfe_info dvbs2_info	= {
	.name					= "Conexant CX24116 DVB-S2",
	.delivery				= DVBFE_DELSYS_DVBS2,
5A5A	.delsys					= {
		.dvbs2.modulation		= DVBFE_MOD_QPSK | DVBFE_MOD_8PSK,

		/* TODO: Review these */
		.dvbs2.fec			= DVBFE_FEC_1_4	| DVBFE_FEC_1_3 |
				  		  DVBFE_FEC_2_5	| DVBFE_FEC_1_2 |
				  		  DVBFE_FEC_3_5	| DVBFE_FEC_2_3 |
				  		  DVBFE_FEC_3_4	| DVBFE_FEC_4_5 |
				  		  DVBFE_FEC_5_6	| DVBFE_FEC_8_9 |
				  		  DVBFE_FEC_9_10,

	},

	.frequency_min		= 950000,
	.frequency_max		= 2150000,
	.frequency_step		= 0,
	.symbol_rate_min	= 1000000,
	.symbol_rate_max	= 45000000,
	.symbol_rate_tolerance	= 0
};
#endif
static int cx24116_set_params(struct dvb_frontend* fe, struct dvbfe_params *p)
{
	struct cx24116_state *state = fe->demodulator_priv;
	struct cx24116_cmd cmd;
	int ret;
	u8 val;
	u8 fec_val;

	dprintk("%s()\n",__FUNCTION__);

	state->dnxt.delivery = p->delivery;
	state->dnxt.frequency = p->frequency;

	if ((ret = cx24116_set_inversion(state, p->inversion)) !=  0)
		return ret;

	if ((ret = cx24116_set_fec(state, p)) !=  0)
		return ret;

	if ((ret = cx24116_set_symbolrate(state, p)) !=  0)
		return ret;

	state->dcur.rolloff = p->delsys.dvbs.rolloff;

	/* discard the 'current' tuning parameters and prepare to tune */
	cx24116_clone_params(fe);

	// Ignore pilot setting for S
	if (state->dcur.pilot || state->dcur.delivery==DVBFE_DELSYS_DVBS)
		fec_val =  state->dcur.fec_val;
	else
		fec_val =  state->dcur.fec_val | 0x40;

	dprintk("%s:   frequency   = %d\n", __FUNCTION__, state->dcur.frequency);
	dprintk("%s:   symbol_rate = %d\n", __FUNCTION__, state->dcur.symbol_rate);
	dprintk("%s:   FEC         = %d (mask/val = 0x%02x/0x%02x)\n", __FUNCTION__,
		state->dcur.fec, state->dcur.fec_mask,  fec_val);
	dprintk("%s:   Inversion   = %d (val = 0x%02x)\n", __FUNCTION__,
		state->dcur.inversion, state->dcur.inversion_val);

	if (state->config->set_ts_params)
		state->config->set_ts_params(fe, 0);

	/* Reset status register */
	val=cx24116_readreg(state, 0x9d);
	val&= 0xc0;
	cx24116_writereg(state, 0x9d, val);

	cmd.id = CMD_BANDWIDTH;
	cmd.args[1]=1;
	ret = cx24116_cmd_execute(fe, &cmd);
	if (ret)
		return ret;

	/* Prepare a tune request */
	cmd.id = CMD_TUNEREQUEST;

	/* Frequency */
	cmd.args[0x01] = (state->dcur.frequency & 0xff0000) >> 16;
	cmd.args[0x02] = (state->dcur.frequency & 0x00ff00) >> 8;
	cmd.args[0x03] = (state->dcur.frequency & 0x0000ff);

	/* Symbol Rate */
	cmd.args[0x04] = ((state->dcur.symbol_rate / 1000) & 0xff00) >> 8;
	cmd.args[0x05] = ((state->dcur.symbol_rate / 1000) & 0x00ff);

	/* Automatic Inversion */
	cmd.args[0x06] =  state->dcur.inversion_val;

	/* Modulation / FEC */

	cmd.args[0x07] =  fec_val;

	cmd.args[0x08] = CX24116_SEARCH_RANGE_KHZ >> 8;
	cmd.args[0x09] = CX24116_SEARCH_RANGE_KHZ & 0xff;
	cmd.args[0x0a] = 0x00;
	cmd.args[0x0b] = 0x00;

	if (state->dcur.rolloff == DVBFE_ROLLOFF_25)
		cmd.args[0x0c] = 0x01;
	else if (state->dcur.rolloff == DVBFE_ROLLOFF_20)
		cmd.args[0x0c] = 0x00;
	else
		cmd.args[0x0c] = 0x02; // 0.35

	cmd.args[0x0d] = state->dcur.fec_mask;


	cx24116_writereg(state, 0xF9, 0x00);
	cx24116_writereg(state, 0xF3, 0x46);

	cmd.args[0x0e] = 0x06;
	cmd.args[0x0f] = 0x00;
	cmd.args[0x10] = 0x00;
	cmd.args[0x11] = 0xFA; 
	cmd.args[0x12] = 0x24;

	ret = cx24116_cmd_execute(fe, &cmd);
	if (ret)
		return ret;

	cmd.id = CMD_BANDWIDTH;
	cmd.args[1]=0;
	ret = cx24116_cmd_execute(fe, &cmd);

	return ret;
}

/* Why isn't this part of the frontend core? */
/* This could be generic and set_params gets callvia via the ops */
static int cx24116_set_frontend(struct dvb_frontend* fe, struct dvb_frontend_parameters *p)
{
	struct dvbfe_params newfe;
	int ret = 0;
	
	dprintk("%s\n", __FUNCTION__);

	ret = cx24116_create_new_qpsk_feparams(fe, p, &newfe);
	if(ret != 0)
		return ret;

	return cx24116_set_params(fe, &newfe);
}


static int cx24116_get_params(struct dvb_frontend* fe,
			      struct dvbfe_params *p)
{
	struct cx24116_state *state = fe->demodulator_priv;
	int ret = 0;
	s16 frequency_offset;
	s16 sr_offset;

	dprintk("%s()\n",__FUNCTION__);
	
	frequency_offset=(cx24116_readreg(state, 0x9f)<<8)+cx24116_readreg(state, 0xa0);
	sr_offset= 0 ; //(cx24116_readreg(state, 0xa1)<<8)+cx24116_readreg(state, 0xa2);

	p->frequency = state->dcur.frequency + frequency_offset/2; // unit seems to be 2kHz

	if (cx24116_get_inversion(state, &p->inversion) != 0) {
		printk("%s: Failed to get inversion status\n",__FUNCTION__);
		return -EREMOTEIO;
	}

	p->delivery = state->dcur.delivery;

	switch(p->delivery)
	{
		case DVBFE_DELSYS_DVBS2:
			cx24116_get_fec_dvbs2(state, &p->delsys.dvbs2.fec);
			p->delsys.dvbs2.symbol_rate = state->dcur.symbol_rate + sr_offset*1000;
			break;
		case DVBFE_DELSYS_DVBS:
			cx24116_get_fec_dvbs(state, &p->delsys.dvbs.fec);
			p->delsys.dvbs.symbol_rate = state->dcur.symbol_rate + sr_offset*1000;
			break;
		default:
			ret = -ENOTSUPP;
	}

	return ret;
}

/* Why isn't this a generic frontend core function? */
/* This could be generic and get_params gets callvia via the ops */
static int cx24116_get_frontend(struct dvb_frontend* fe, struct dvb_frontend_parameters *p)
{
	struct dvbfe_params feparams;
	int ret;
	dprintk("%s()\n",__FUNCTION__);

	ret = cx24116_get_params(fe, &feparams);
	if(ret != 0)
		return ret;

	return cx24116_create_old_qpsk_feparams(fe, &feparams, p);
}
#if 0
static int cx24116_get_info(struct dvb_frontend *fe,
			    struct dvbfe_info *fe_info)
{
	dprintk("%s()\n",__FUNCTION__);

        switch (fe_info->delivery)
	{
        case DVBFE_DELSYS_DVBS:
		memcpy(fe_info, &dvbs_info, sizeof (dvbs_info));
                break;
        case DVBFE_DELSYS_DVBS2:
                memcpy(fe_info, &dvbs2_info, sizeof (dvbs2_info));
                break;
        default:
		dprintk("%s() invalid arg\n",__FUNCTION__);
		return -EINVAL;
        }               
                
        return 0;       
}

/* TODO: The hardware does DSS too, how does the kernel demux handle this? */
static int cx24116_get_delsys(struct dvb_frontend *fe,
	enum dvbfe_delsys *fe_delsys)
{
	dprintk("%s()\n",__FUNCTION__);
	*fe_delsys = DVBFE_DELSYS_DVBS | DVBFE_DELSYS_DVBS2;

	return 0;
}

/* TODO: is this necessary? */
static enum dvbfe_algo cx24116_get_algo(struct dvb_frontend *fe)
{
	dprintk("%s()\n",__FUNCTION__);
	return DVBFE_ALGO_RECOVERY;
}
#endif
static struct dvb_frontend_ops cx24116_ops = {

	.info = {
		.name = "Conexant CX24116 DVB-S2",
		.type = FE_QPSK,
		.frequency_min = 950000,
		.frequency_max = 2150000,
		.frequency_stepsize = 1011, /* kHz for QPSK frontends */
		.frequency_tolerance = 5000,
		.symbol_rate_min = 1000000,
		.symbol_rate_max = 45000000,

		.caps = /* FE_CAN_INVERSION_AUTO | */  // (ab)use inversion for pilot tones
			FE_CAN_FEC_1_2 | FE_CAN_FEC_2_3 | FE_CAN_FEC_3_4 |
			FE_CAN_FEC_4_5 | FE_CAN_FEC_5_6 | FE_CAN_FEC_6_7 |
			FE_CAN_FEC_7_8 | FE_CAN_FEC_AUTO |
			FE_CAN_QPSK | FE_CAN_RECOVER
	},

	.release = cx24116_release,

	.init = cx24116_initfe,
	.sleep = cx24116_sleep,
	.set_frontend = cx24116_set_frontend,
	.get_frontend = cx24116_get_frontend,
	.read_status = cx24116_read_status,
	.read_ber = cx24116_read_ber,
	.read_signal_strength = cx24116_read_signal_strength,
	.read_snr = cx24116_read_snr,
	.read_ucblocks = cx24116_read_ucblocks,
	.set_tone = cx24116_set_tone,
	.diseqc_send_master_cmd = cx24116_send_diseqc_msg,
        .diseqc_send_burst = cx24116_send_diseqc_burst,
	.set_voltage = cx24116_set_voltage,
#if 0
	.set_params		= cx24116_set_params,
	.get_params		= cx24116_get_params,
	.get_info		= cx24116_get_info,
	.get_delsys		= cx24116_get_delsys,
	.get_frontend_algo	= cx24116_get_algo,
#endif	
};

module_param(debug, int, 0644);
MODULE_PARM_DESC(debug, "Activates frontend debugging (default:0)");

MODULE_DESCRIPTION("DVB Frontend module for Conexant cx24116/cx24118 hardware");
MODULE_AUTHOR("Steven Toth, Georg Acher");
MODULE_LICENSE("GPL");

EXPORT_SYMBOL(cx24116_attach);
