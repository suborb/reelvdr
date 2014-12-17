#/*****************************************************************************/
/*
 * I2C interface for Reelbox FPGA (4 tuner version)
 *
 * Copyright (C) 2004-2005  Georg Acher, BayCom GmbH (acher at baycom dot de)
 *
 *
 * #include <GPL-header>
 *
 *  $Id: reel_i2c.c,v 1.2 2005/09/10 14:34:16 acher Exp $
 *
 */       
/*****************************************************************************/

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/string.h>
#include <linux/timer.h>
#include <linux/sched.h>
#include <linux/smp_lock.h>

#include <asm/io.h>

#include <linux/delay.h>
#include "reeldvb.h"

#define dbg(format, arg...) printk(format"\n", ## arg)
#define err(format, arg...) printk(format"\n", ## arg)

/* ------------------------------------------------------------------------- */
// Low Level I2C calls
/* ------------------------------------------------------------------------- */

void reel_i2c_sda(struct reelfpga *rfpga, int bus, int val)
{
	int v=0,mask;
	unsigned long flags;

	if (val) val=1;
	spin_lock_irqsave(&rfpga->i2c_lock,flags);
	mask=(REEL_I2C_SDA1<<(bus*2));
	v=readl(rfpga->regbase+REEL_GPIO_I2C);
	v=(v&~mask)|(val*mask);
	writel(v,rfpga->regbase+REEL_GPIO_I2C);
	spin_unlock_irqrestore(&rfpga->i2c_lock,flags);
}
/* ------------------------------------------------------------------------- */
void reel_i2c_scl(struct reelfpga *rfpga, int bus, int val)
{
	int v=0,mask;
	unsigned long flags;
	if (val) val=1;
	spin_lock_irqsave(&rfpga->i2c_lock,flags);
	mask=(REEL_I2C_SCL1<<(bus*2));
	v=readl(rfpga->regbase+REEL_GPIO_I2C);
	v=(v&~mask)|(val*mask);
	writel(v,rfpga->regbase+REEL_GPIO_I2C);
	spin_unlock_irqrestore(&rfpga->i2c_lock,flags);
}
/* ------------------------------------------------------------------------- */
int reel_i2c_read_sda(struct reelfpga *rfpga, int bus)
{
	int val;
	val=readl(rfpga->regbase+REEL_GPIO_I2C);
	return REEL_I2C_RSDA(val,bus);
}
/* ------------------------------------------------------------------------- */
int reel_i2c_read_scl(struct reelfpga *rfpga, int bus)
{
	int val;
	val=readl(rfpga->regbase+REEL_GPIO_I2C);
	return REEL_I2C_RSCL(val,bus);
}
/* ------------------------------------------------------------------------- */
// Bitbanging algo calls
/* ------------------------------------------------------------------------- */
void reel_setscl(void *data, int state)
{
	struct reel_unit* ru=(struct reel_unit*)data;

	reel_i2c_scl(ru->rfpga, ru->hw_unit,state);
}
/* ------------------------------------------------------------------------- */
void reel_setsda(void *data, int state)
{
	struct reel_unit* ru=(struct reel_unit*)data;

	reel_i2c_sda(ru->rfpga, ru->hw_unit,state);
}
/* ------------------------------------------------------------------------- */
int reel_getsda(void* data)
{
	struct reel_unit* ru=(struct reel_unit*)data;

	return reel_i2c_read_sda(ru->rfpga, ru->hw_unit);	
}
/* ------------------------------------------------------------------------- */
// Yep, we can read back SDA _and_ SCL!

int reel_getscl(void* data)
{
	struct reel_unit* ru=(struct reel_unit*)data;

	return reel_i2c_read_scl(ru->rfpga, ru->hw_unit);
}
/* ------------------------------------------------------------------------- */
// I2C layer stuff
/* ------------------------------------------------------------------------- */
static struct i2c_algo_bit_data reel_i2c_data = {
        .setsda         = reel_setsda,
        .setscl         = reel_setscl,
        .getsda         = reel_getsda,
        .getscl         = reel_getscl,
        .udelay         = 10,  
        .mdelay         = 80,
        .timeout        = HZ/10
};

static struct i2c_adapter reel_i2c_ops = {
        .owner          = THIS_MODULE,
        .id             = I2C_HW_B_LP, // quite close...
        .name           = "Reel-FPGA",
	.class = I2C_CLASS_TV_DIGITAL
};
/* ------------------------------------------------------------------------- */
int reel_i2c_register(struct reel_unit *ru)
{
	writel(0xff,ru->rfpga->regbase+REEL_GPIO_I2C);

	ru->adapter=reel_i2c_ops;	
	ru->algo_data=reel_i2c_data;
	ru->algo_data.data=ru;
	ru->adapter.algo_data = &ru->algo_data;
	sprintf(ru->adapter.name, "Reel I2C %i",ru->num);

	i2c_set_adapdata(&ru->adapter,ru);

	if (i2c_bit_add_bus(&ru->adapter) < 0) {
                err("reel_i2c_register: Unable to register I2C for unit %i", ru->num);
                return -ENOMEM;
        }
	printk("Registered I2C-Bus %i\n",ru->num);
	ru->i2c_used=1;
	return 0;
}
/* ------------------------------------------------------------------------- */
void reel_i2c_unregister(struct reel_unit *ru)
{
	printk("Unregistered I2C-Bus %i\n",ru->num);
	i2c_bit_del_bus(&ru->adapter);
}
/* ------------------------------------------------------------------------- */
// set tuner power...
int reel_lnbp21_write(struct  reel_unit *ru, u8 data)
{
        int ret;
        u8 buf [] = {  data };
        struct i2c_msg msg = { .addr = 0x10/2, .flags = 0, .buf = buf, .len = 1 };

	ret=i2c_transfer(&ru->adapter,&msg,1);

        return (ret != 1) ? -1 : 0;
}
/* ------------------------------------------------------------------------- */
