/* dvbloop - A DVB Loopback Device
 * Copyright (C) 2006 Christian Praehauser, Deti Fliegl
 -----------------------------------------
 * File: dvblo_adap.c
 * Desc: Support for virtual DVB adapters - Frontend implementation
 * Date: October 2006
 * Author: Christian Praehauser <cpreahaus@cosy.sbg.ac.at>, Deti Fliegl <deti@fliegl.de>
 *
 * This file is released under the GPLv2.
 */

/* avoid definition of __module_kernel_version in the resulting object file */
#define __NO_VERSION__

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/spinlock.h>
#include <linux/init.h>
#include <linux/errno.h>
#include <linux/version.h>

#include "dvblo.h"
#include "dvblo_adap.h"

#define DBGLEV_ADAP_FE1		(DBGLEV_1<<DBGLEV_ADAP_FE)
#define DBGLEV_ADAP_FE2		(DBGLEV_2<<DBGLEV_ADAP_FE)
#define DBGLEV_ADAP_FE3		(DBGLEV_3<<DBGLEV_ADAP_FE)

#define NETCEIVER_FEC_3_5  13
#define NETCEIVER_FEC_9_10 14
#define NETCEIVER_QPSK_S2   9 
#define NETCEIVER_PSK8     10

#if 0
struct dvb_frontend_parameters
{
	__u32 frequency;	/* (absolute) frequency in Hz for QAM/OFDM/ATSC */

	/* intermediate frequency in kHz for QPSK */
	fe_spectral_inversion_t inversion;
	union
	{
		struct dvb_qpsk_parameters qpsk;
		struct dvb_qam_parameters qam;
		struct dvb_ofdm_parameters ofdm;
		struct dvb_vsb_parameters vsb;
	} u;
};

struct dvb_frontend_tune_settings
{
	int min_delay_ms;
	int step_size;
	int max_drift;
	struct dvb_frontend_parameters parameters;
};

#endif /*  */

/* fontend ops */
int dvblo_adap_fe_get_frontend (struct dvb_frontend *fe, struct dvb_frontend_parameters *params)
{
	int rv = SUCCESS;
	struct dvblo *dvblo = FE_PRIV (fe);
	dprintk (DBGLEV_ADAP_FE3, "fe=%p\n", fe);
	if (dvblo == NULL)
		rv = -EINVAL;

	else {
		*params = dvblo->fe.params;
	}
	return rv;
}

int dvblo_adap_fe_set_frontend (struct dvb_frontend *fe, struct dvb_frontend_parameters *params)
{
	int rv = SUCCESS;
	struct dvblo *dvblo = FE_PRIV (fe);
	dprintk (DBGLEV_ADAP_FE3, "fe=%p\n", fe);
	if (dvblo == NULL)
		rv = -EINVAL;

	else {
		dvblo->fe.params = *params;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,28)
		if (dvblo_netceiver) {
			struct dtv_frontend_properties *c =
				&fe->dtv_property_cache;

			if (c->delivery_system == SYS_DVBS2) {
				switch ((int)c->fec_inner) {
				case FEC_3_5:
					dvblo->fe.params.u.qpsk.fec_inner =
						NETCEIVER_FEC_3_5;
					break;
				case FEC_9_10:
					dvblo->fe.params.u.qpsk.fec_inner =
						NETCEIVER_FEC_9_10;
					break;
				}
				switch ((int)c->modulation) {
				case QPSK:
					dvblo->fe.params.u.qpsk.fec_inner |=
						NETCEIVER_QPSK_S2 << 16;
					break;
				case PSK_8:
					dvblo->fe.params.u.qpsk.fec_inner |=
						NETCEIVER_PSK8 << 16;
					break;
				}
			}
		}
#endif
		dvblo_set_event (dvblo, EV_FRONTEND);
	}
	return rv;
}

int dvblo_adap_fe_get_tune_settings (struct dvb_frontend *fe, struct dvb_frontend_tune_settings *settings)
{
	int rv = SUCCESS;
	struct dvblo *dvblo = FE_PRIV (fe);
	dprintk (DBGLEV_ADAP_FE3, "fe=%p\n", fe);
	if (dvblo == NULL)
		rv = -EINVAL;

	else {
		settings->min_delay_ms = 2000;
		settings->step_size = 0;
		settings->max_drift = 0;
		settings->parameters = dvblo->fe.params;
	}
	return rv;
}

#if 0
typedef enum fe_status
{ FE_HAS_SIGNAL = 0x01,		/*  found something above the noise level */
	FE_HAS_CARRIER = 0x02,	/*  found a DVB signal  */
	FE_HAS_VITERBI = 0x04,	/*  FEC is stable  */
	FE_HAS_SYNC = 0x08,	/*  found sync bytes  */
	FE_HAS_LOCK = 0x10,	/*  everything's working... */
	FE_TIMEDOUT = 0x20,	/*  no lock within the last ~2 seconds */
	FE_REINIT = 0x40	/*  frontend was reinitialized,  */
} fe_status_t;			/*  application is recommended to reset */

	/*  DiSEqC, tone and parameters */
#endif /*  */

int dvblo_adap_fe_read_status (struct dvb_frontend *fe, fe_status_t * status)
{
	int rv = SUCCESS;
	struct dvblo *dvblo = FE_PRIV (fe);
	dprintk (DBGLEV_ADAP_FE3, "fe=%p\n", fe);
	if (dvblo == NULL)
		rv = -EINVAL;

	else {
		*status = dvblo->fe.status.st;
	}
	return rv;
}

int dvblo_adap_fe_read_ber (struct dvb_frontend *fe, u32 * ber)
{
	int rv = SUCCESS;
	struct dvblo *dvblo = FE_PRIV (fe);
	dprintk (DBGLEV_ADAP_FE3, "fe=%p\n", fe);
	if (dvblo == NULL)
		rv = -EINVAL;

	else {
		*ber = dvblo->fe.status.ber;
	}
	return rv;
}

int dvblo_adap_fe_read_signal_strength (struct dvb_frontend *fe, u16 * strength)
{
	int rv = SUCCESS;
	struct dvblo *dvblo = FE_PRIV (fe);
	dprintk (DBGLEV_ADAP_FE3, "fe=%p\n", fe);
	if (dvblo == NULL)
		rv = -EINVAL;

	else {
		*strength = dvblo->fe.status.strength;
	}
	return rv;
}

int dvblo_adap_fe_read_snr (struct dvb_frontend *fe, u16 * snr)
{
	int rv = SUCCESS;
	struct dvblo *dvblo = FE_PRIV (fe);
	dprintk (DBGLEV_ADAP_FE3, "fe=%p\n", fe);
	if (dvblo == NULL)
		rv = -EINVAL;

	else {
		*snr = dvblo->fe.status.snr;
	}
	return rv;
}

int dvblo_adap_fe_read_ucblocks (struct dvb_frontend *fe, u32 * ucblocks)
{
	int rv = SUCCESS;
	struct dvblo *dvblo = FE_PRIV (fe);
	dprintk (DBGLEV_ADAP_FE3, "fe=%p\n", fe);
	if (dvblo == NULL)
		rv = -EINVAL;

	else {
		*ucblocks = dvblo->fe.status.ucblocks;
	}
	return rv;
}

static int dvblo_send_diseqc_msg (struct dvb_frontend *fe, struct dvb_diseqc_master_cmd *m)
{
	int rv = SUCCESS;
	struct dvblo *dvblo = FE_PRIV (fe);
	dprintk (DBGLEV_ADAP_FE3, "fe=%p\n", fe);
	if (dvblo == NULL)
		rv = -EINVAL;

	else {
		dvblo->fe.sec.diseqc_cmd = *m;
		dvblo_set_event (dvblo, EV_DISEC_MSG);
	}
	return rv;
}

static int dvblo_send_diseqc_burst (struct dvb_frontend *fe, fe_sec_mini_cmd_t burst)
{
	int rv = SUCCESS;
	struct dvblo *dvblo = FE_PRIV (fe);
	dprintk (DBGLEV_ADAP_FE3, "fe=%p\n", fe);
	if (dvblo == NULL)
		rv = -EINVAL;

	else {
		dvblo->fe.sec.mini_cmd = burst;
		dvblo_set_event (dvblo, EV_DISEC_BURST);
	}
	return rv;
}

static int dvblo_set_tone (struct dvb_frontend *fe, fe_sec_tone_mode_t tone)
{
	int rv = SUCCESS;
	struct dvblo *dvblo = FE_PRIV (fe);
	dprintk (DBGLEV_ADAP_FE3, "fe=%p\n", fe);
	if (dvblo == NULL)
		rv = -EINVAL;

	else {
		dvblo->fe.sec.tone_mode = tone;
		dvblo_set_event (dvblo, EV_TONE);
	}
	return rv;
}

static int dvblo_set_voltage (struct dvb_frontend *fe, fe_sec_voltage_t voltage)
{
	int rv = SUCCESS;
	struct dvblo *dvblo = FE_PRIV (fe);
	dprintk (DBGLEV_ADAP_FE3, "fe=%p\n", fe);
	if (dvblo == NULL)
		rv = -EINVAL;

	else {
		dvblo->fe.sec.voltage = voltage;
		dvblo_set_event (dvblo, EV_VOLTAGE);
	}
	return rv;
}


/* NOTE: In version 2.6.18, the Tuner was separated from the frontend.
 */

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,18)

/* tuner ops */

/** This is for simple PLLs - set all parameters in one go. */
int dvblo_adap_fe_tuner_set_params (struct dvb_frontend *fe, struct dvb_frontend_parameters *p)
{
	int rv = SUCCESS;
	struct dvblo *dvblo = FE_PRIV (fe);
	dprintk (DBGLEV_ADAP_FE3, "fe=%p\n", fe);
	if (dvblo == NULL)
		rv = -EINVAL;

	else {
		dvblo->fe.tuner.params = *p;
		dvblo_set_event (dvblo, EV_TUNER);
	}
	return rv;
}

/** This is support for demods like the mt352 - fills out the supplied buffer with what to write. */
//int dvblo_adap_fe_tuner_calc_regs(struct dvb_frontend *fe, struct dvb_frontend_parameters *p, u8 *buf, int buf_len);
int dvblo_adap_fe_tuner_get_frequency (struct dvb_frontend *fe, u32 * frequency)
{
	int rv = SUCCESS;
	struct dvblo *dvblo = FE_PRIV (fe);
	dprintk (DBGLEV_ADAP_FE3, "fe=%p\n", fe);
	if (dvblo == NULL)
		rv = -EINVAL;

	else {
		*frequency = dvblo->fe.tuner.params.frequency;
	}
	return rv;
}

int dvblo_adap_fe_tuner_set_frequency (struct dvb_frontend *fe, u32 frequency)
{
	int rv = SUCCESS;
	struct dvblo *dvblo = FE_PRIV (fe);
	dprintk (DBGLEV_ADAP_FE3, "fe=%p\n", fe);
	if (dvblo == NULL)
		rv = -EINVAL;

	else {
		dvblo->fe.tuner.params.frequency = frequency;
		dvblo_set_event (dvblo, EV_FREQUENCY);
	}
	return rv;
}

#if 0
typedef enum fe_bandwidth
{ BANDWIDTH_8_MHZ, BANDWIDTH_7_MHZ, BANDWIDTH_6_MHZ, BANDWIDTH_AUTO
} fe_bandwidth_t;

#endif /*  */

int dvblo_adap_fe_tuner_get_bandwidth (struct dvb_frontend *fe, u32 * bandwidth)
{
	int rv = SUCCESS;
	struct dvblo *dvblo = FE_PRIV (fe);
	dprintk (DBGLEV_ADAP_FE3, "fe=%p\n", fe);
	if (dvblo == NULL)
		rv = -EINVAL;

	else {
		*bandwidth = dvblo->fe.tuner.params.u.ofdm.bandwidth;
	}
	return rv;
}

int dvblo_adap_fe_tuner_set_bandwidth (struct dvb_frontend *fe, u32 bandwidth)
{
	int rv = SUCCESS;
	struct dvblo *dvblo = FE_PRIV (fe);
	dprintk (DBGLEV_ADAP_FE3, "fe=%p\n", fe);
	if (dvblo == NULL)
		rv = -EINVAL;

	else {
		dvblo->fe.tuner.params.u.ofdm.bandwidth = bandwidth;
		dvblo_set_event (dvblo, EV_BANDWIDTH);
	}
	return rv;
}

int dvblo_adap_fe_tuner_get_status (struct dvb_frontend *fe, u32 * status)
{
	int rv = SUCCESS;
	struct dvblo *dvblo = FE_PRIV (fe);
	dprintk (DBGLEV_ADAP_FE3, "fe=%p\n", fe);
	if (dvblo == NULL)
		rv = -EINVAL;

	else {
		*status = dvblo->fe.tuner.status;
	}
	return rv;
}
#endif /*  */
struct dvb_frontend_ops dvblo_adap_fe_ops = {
	.info =	{
		.name = "Virtual DVB Frontend",
		.type = FE_QPSK,
		.frequency_min = 0,
		.frequency_max = 2000000000,
		.frequency_stepsize = 1000,
		.frequency_tolerance = 500,
		.symbol_rate_min = 0,
		.symbol_rate_max = 50000000,
		.symbol_rate_tolerance = 250,
		.caps = FE_IS_STUPID
		},
	.release = NULL,
	.init = NULL,
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,18)
	/* Set to NULL or return 0 for SW-bases ZigZag algorithm
	 * But this causes kernel oops in Linux versions!
	 * get_frontend was added in 2.6.18
	 */
	.get_frontend_algo = NULL,
#endif /*  */
	/* these two are only used for the swzigzag code */
	.set_frontend = dvblo_adap_fe_set_frontend,
	.get_tune_settings = dvblo_adap_fe_get_tune_settings,
	.get_frontend = dvblo_adap_fe_get_frontend,
	.read_status = dvblo_adap_fe_read_status,
	.read_ber = dvblo_adap_fe_read_ber,
	.read_signal_strength = dvblo_adap_fe_read_signal_strength,
	.read_snr = dvblo_adap_fe_read_snr,
	.read_ucblocks = dvblo_adap_fe_read_ucblocks,
	.diseqc_send_master_cmd = dvblo_send_diseqc_msg,
	.diseqc_send_burst = dvblo_send_diseqc_burst,
	.set_tone = dvblo_set_tone,
	.set_voltage = dvblo_set_voltage,
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,18)
	.tuner_ops = 	{
			.info = {
				.name = "Virtual DVB Tuner",
				.frequency_min = 0,
				.frequency_max = 2000000000,
				.frequency_step = 1000,
				.bandwidth_min = 0,
				.bandwidth_max = 20000000,
				.bandwidth_step = 500
				},
			.release = NULL,
			.init = NULL,
			.set_params = dvblo_adap_fe_tuner_set_params,
			.get_frequency = dvblo_adap_fe_tuner_get_frequency,
			.get_bandwidth = dvblo_adap_fe_tuner_get_bandwidth,
			.get_status = dvblo_adap_fe_tuner_get_status,
			.set_frequency = dvblo_adap_fe_tuner_set_frequency,
			.set_bandwidth = dvblo_adap_fe_tuner_set_bandwidth
			}
#endif /*  */
};

int dvblo_fe_get_info (struct dvblo *dvblo, struct dvb_frontend_info *info)
{
	struct dvb_frontend_ops *ops;
	int rv = SUCCESS;
	if (dvblo == NULL) {
		rv = -EINVAL;
		return rv;
	}
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,18)
	ops = &dvblo->dvb.frontend.ops;

#else /*  */
	ops = dvblo->dvb.frontend.ops;

#endif /*  */
	*info = ops->info;
	return rv;
}

int dvblo_fe_set_info (struct dvblo *dvblo, struct dvb_frontend_info *info)
{
	struct dvb_frontend_ops *ops;
	int rv = SUCCESS;
	if (dvblo == NULL) {
		rv = -EINVAL;
		return rv;
	}
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,18)
	ops = &dvblo->dvb.frontend.ops;

#else /*  */
	ops = dvblo->dvb.frontend.ops;

#endif /*  */
	ops->info = *info;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,29)
	if (dvblo_netceiver && strstr(info->name, "DVB-S2"))
		ops->info.caps |= FE_CAN_2G_MODULATION;
#endif
	return rv;
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,18)
int dvblo_fe_get_tunerinfo (struct dvblo *dvblo, struct dvb_tuner_info *tunerinfo)
{
	struct dvb_frontend_ops *ops;

	int rv = SUCCESS;
	if (dvblo == NULL) {
		rv = -EINVAL;
		return rv;
	}
	ops = &dvblo->dvb.frontend.ops;
	*tunerinfo = ops->tuner_ops.info;
	return rv;
}

int dvblo_fe_set_tunerinfo (struct dvblo *dvblo, struct dvb_tuner_info *tunerinfo)
{
	struct dvb_frontend_ops *ops;

	int rv = SUCCESS;
	if (dvblo == NULL) {
		rv = -EINVAL;
		return rv;
	}
	ops = &dvblo->dvb.frontend.ops;
	ops->tuner_ops.info = *tunerinfo;
	return rv;
}
#endif /*  */
