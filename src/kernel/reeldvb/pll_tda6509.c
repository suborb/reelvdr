/*****************************************************************************/
/*
 * TDA6509-PLL driver for Philips CU1216
 * 
 * (c) 2005 Georg Acher, BayCom GmbH (acher at baycom dot de)
 *
 * Extracted from dvb-ttusb-budget.c
 *
 * (c) by
 * Holger Waechtler <holger@convergence.de>
 * Felix Domke <tmbinc@elitedvb.net>
 *
 * #include <gpl-header>
 *
 * $Id: pll_tda6509.c,v 1.2 2005/09/10 14:34:16 acher Exp $
 */
/*****************************************************************************/

#include "tda10023.h"

static int philips_cu1216_pll_set(struct dvb_frontend *fe, struct dvb_frontend_parameters *params)
{
        struct reel_unit *ru = (struct reel_unit *) fe->dvb->priv;
        u8 buf[6];
        struct i2c_msg msg = {.addr = 0x60,.flags = 0,.buf = buf,.len = sizeof(buf) };

#define CU_1216_IF 36125000
#define TUNER_MUL 62500

        u32 div = (params->frequency + CU_1216_IF + TUNER_MUL / 2) / TUNER_MUL;

        buf[0] = (div >> 8) & 0x7f;
        buf[1] = div & 0xff;
        buf[2] = 0xce;
        buf[3] = (params->frequency < (160000000+CU_1216_IF) ? 0x01 :
                  params->frequency >=  (445000000+CU_1216_IF) ? 0x04 
		  : 0x02);

	buf[4] = 0xde;
	buf[5] = 0x20;

        if (i2c_transfer(&ru->adapter, &msg, 1) != 1)
                return -EIO;
        return 0;
}

static u8 cu1216_read_pwm(struct reel_unit *ru)
{
        u8 b = 0xff;
        u8 pwm;
        struct i2c_msg msg[] = { {.addr = 0x50,.flags = 0,.buf = &b,.len = 1},
        {.addr = 0x50,.flags = I2C_M_RD,.buf = &pwm,.len = 1}
        };

/*        if ((i2c_transfer(&ru->adapter, msg, 2) != 2)
            || (pwm == 0xff))

                pwm = 0x48;
*/
	pwm=0x48;

        return pwm;
}



static struct tda10023_config philips_cu1216_config = {
        .demod_address = 0x18/2,
        .pll_set = philips_cu1216_pll_set,
};
