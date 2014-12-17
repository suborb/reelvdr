/*
  Driver for the ST STV0288 channel decoder

  (c) 2006 Georg Acher, BayCom GmbH, acher (at) baycom (dot) de
	  for Reel Multimedia

  #include <gpl_v2.h>

*/

#ifndef STV0288_H
#define STV0288_H

#include <linux/dvb/frontend.h>
#include "dvb_frontend.h"

struct stv0288_config
{
        u8 demod_address;

        /* master clock to use */
        u32 mclk;

        /* does the inversion require inversion? */
        u8 invert:1;

        /* minimum delay before retuning */
        int min_delay_ms;

        /* Set the symbol rate */
        int (*set_symbol_rate)(struct dvb_frontend* fe, u32 srate, u32 ratio);

        /* PLL maintenance */
        int (*pll_init)(struct dvb_frontend* fe);
        int (*pll_set)(struct dvb_frontend* fe, struct dvb_frontend_parameters* params);
};

struct stv0288_state {
        struct i2c_adapter* i2c;
        struct dvb_frontend_ops ops;
        const struct stv0288_config* config;
        struct dvb_frontend frontend;

        u8 initialised:1;

        u32 tuner_frequency;
        u32 symbol_rate;
        fe_code_rate_t fec_inner;
        int errmode;
};

extern int stv0288_sleep(struct dvb_frontend* fe);

extern int stv0288_writereg (struct dvb_frontend* fe, u8 reg, u8 data);

extern struct dvb_frontend* stv0288_attach(const struct stv0288_config* config,
                                           struct i2c_adapter* i2c);

#endif
