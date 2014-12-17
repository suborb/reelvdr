/*
    TDA10023 - DVB-C decoder (as used in Philips CU1216-3 NIM)

    Copyright (C) 2005 Georg Acher, BayCom GmbH (acher at baycom dot de)

    Based on tda10021.c 
    Copyright (C) 1999 Convergence Integrated Media GmbH <ralph@convergence.de>
    Copyright (C) 2004 Markus Schulz <msc@antzsystem.de>
                   Support for TDA10021

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

#ifndef TDA10023_H
#define TDA10023_H

#include <linux/dvb/frontend.h>

struct tda10023_config
{
	/* the demodulator's i2c address */
	u8 demod_address;

	/* PLL maintenance */
	int (*pll_init)(struct dvb_frontend* fe);
	int (*pll_set)(struct dvb_frontend* fe, struct dvb_frontend_parameters* params);
};

extern struct dvb_frontend* tda10023_attach(const struct tda10023_config* config,
					    struct i2c_adapter* i2c, u8 pwm);

#endif // TDA10023_H
