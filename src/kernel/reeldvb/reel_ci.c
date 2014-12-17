/*****************************************************************************/
/*
 * Common Interface physical layer for Reelbox FPGA (4 tuner version)
 *
 * Copyright (C) 2004-2005  Georg Acher, BayCom GmbH (acher at baycom dot de)
 *
 *
 * #include <gpl-header>
 *
 *  $Id: reel_ci.c,v 1.2 2005/09/10 14:34:16 acher Exp $
 *
 */
/*****************************************************************************/

#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/timer.h>
#include <linux/smp_lock.h>
#include <linux/module.h>
#include <linux/init.h>
#include <asm/io.h>
#include <linux/device.h>
#include <linux/wait.h>
#include <linux/delay.h>

#include "reeldvb.h"

static int ce_tab_read[3]={0x3e,0x3b,0x2f}; // 0011|1110   0011|1011  0010|1111
static int ce_tab_write[3]={0xbe,0xbb,0xaf};
static int reset_tab[3]={0x10,0x20,0x40};

#define REEL_CI_OEWR 0xf

// Define if CAM detection and power handling should be done in kernel
//#define CORE_HANDLING
#define CORE_SLOTS 7
/* ------------------------------------------------------------------------- */
void reel_tps2206_send(struct reelfpga *rfpga, int data)
{
	int n,d;

	writel(REEL_PWR_RESET,rfpga->regbase+REEL_GPIO_CIPWR);	
	
	for(n=0;n<9;n++) {
		d=0;
		if (data&256)
			d=REEL_PWR_DATA;
		writel(REEL_PWR_RESET|d,rfpga->regbase+REEL_GPIO_CIPWR);
		writel(REEL_PWR_RESET|d|REEL_PWR_CLK,rfpga->regbase+REEL_GPIO_CIPWR);
		writel(REEL_PWR_RESET|d,rfpga->regbase+REEL_GPIO_CIPWR);	
		data<<=1;
	}
	writel(REEL_PWR_RESET|REEL_PWR_LATCH,rfpga->regbase+REEL_GPIO_CIPWR);
	writel(REEL_PWR_RESET,rfpga->regbase+REEL_GPIO_CIPWR);
}
/* ------------------------------------------------------------------------- */
int reel_ci_power(struct reelfpga *rfpga, int slot, int voltage)
{
	int d,v;
	if (slot<0 || slot>2)
		return -EINVAL;

	down(&rfpga->ci_mutex); 
	d=rfpga->ci_powerstate;
	d|=0x100; // No shutdown

	if (voltage==3)
		voltage=5; // Hack
//	if (voltage==0)
//		voltage=3; // Fix RevB/C Bug

	printk("Setting CI-Slot %i to %iV\n",slot,voltage);

	if (slot==0) {
		if (voltage==3)
			v=0x7; // Slot A: VCC 3V, VPP Hi-Z
		else if (voltage==5)
			v=0xb; // Slot A: VCC 5V, VPP Hi-Z
		else 
			v=0x3;    // Slot A: VCC 0V, VPP Hi-Z
		d=(d&0x1f0)|v;
	}
	else if (slot==1) {
		if (voltage==3)
			v=0xb0; // Slot B: VCC 3V, VPP Hi-Z
		else if (voltage==5)
			v=0x70; // Slot B: VCC 5V, VPP Hi-Z
		else 
			v=0x30;    // Slot B: VCC 0V, VPP Hi-Z	
		d=(d&0x10f)|v;
	}
	reel_tps2206_send(rfpga,d);
	msleep_interruptible(100);
	rfpga->ci_powerstate=d;	
	up(&rfpga->ci_mutex);	
	return 0;
}
/* ------------------------------------------------------------------------- */
int reel_ci_read(struct reelfpga *rfpga, int slot, int addr)
{
	int ret;
	int ctrl_val;
	int ce_val;
	unsigned long flags;

	if (slot<0 || slot>2)
		return -1;

	down(&rfpga->ci_mutex);
	
	ctrl_val=rfpga->ci_resetstate;

	if (addr&0x8000) 
		ctrl_val|=REEL_CI_OEWR-4;		
	else
		ctrl_val|=REEL_CI_OEWR-1; // attribute mem

	ctrl_val|=rfpga->ci_resetstate;
	ce_val=ce_tab_read[slot];

	addr&=0x7fff; // IO also has REG=0

	spin_lock_irqsave(&rfpga->lock, flags); // better lock IRQs for sane timing

	writel(REEL_CI_OE|REEL_CI_CES|0xff, rfpga->regbase+REEL_CI_CA);                   // CE=1
	writel(REEL_CI_OE|REEL_CI_CTRL|REEL_CI_OEWR|ctrl_val, rfpga->regbase+REEL_CI_CA); // OE=1

	writel(REEL_CI_OE|REEL_CI_LOW_ADDR|(addr&0xff), rfpga->regbase+REEL_CI_CA);       // addr low
	writel(REEL_CI_OE|REEL_CI_HIGH_ADDR|((addr>>8)&0xff), rfpga->regbase+REEL_CI_CA); // addr high

	writel(REEL_CI_OE|REEL_CI_CES|ce_val, rfpga->regbase+REEL_CI_CA);                 // CE=0
	writel(REEL_CI_OE|REEL_CI_CTRL|ctrl_val, rfpga->regbase+REEL_CI_CA);              // OE=0

	writel(REEL_CI_OE|REEL_CI_CTRL|REEL_CI_BUFD|ctrl_val, rfpga->regbase+REEL_CI_CA); // latch
	ret=readl(rfpga->regbase+REEL_CI_DATA);
	writel(REEL_CI_OE|REEL_CI_CTRL|REEL_CI_OEWR|ctrl_val, rfpga->regbase+REEL_CI_CA);          // OE=1
	writel(REEL_CI_OE|REEL_CI_CES|0xff, rfpga->regbase+REEL_CI_CA);                   // CE=1

	spin_unlock_irqrestore(&rfpga->lock, flags);
//	printk("READ %i %x %x\n",slot,addr,ret);
	up(&rfpga->ci_mutex);
	return ret;
}
/* ------------------------------------------------------------------------- */
int reel_ci_write(struct reelfpga *rfpga, int slot, int addr, int data)
{
	int ctrl_val;
	unsigned long flags;

	if (slot<0 || slot>2)
		return -1;

	down(&rfpga->ci_mutex);
	ctrl_val=rfpga->ci_resetstate;
	if (addr&0x8000)
		ctrl_val|=REEL_CI_OEWR-8;
	else
		ctrl_val|=REEL_CI_OEWR-2;

	ctrl_val|=rfpga->ci_resetstate;

	addr&=0x7fff; // IO also has REG=0
	spin_lock_irqsave(&rfpga->lock, flags);
	writel(REEL_CI_OE|REEL_CI_CES|0xff, rfpga->regbase+REEL_CI_CA);
	writel(REEL_CI_OE|REEL_CI_CTRL|REEL_CI_OEWR|ctrl_val, rfpga->regbase+REEL_CI_CA); // OE=1

	writel(REEL_CI_OE|REEL_CI_LOW_ADDR|(addr&0xff), rfpga->regbase+REEL_CI_CA); // addr low
	writel(REEL_CI_OE|REEL_CI_HIGH_ADDR|((addr>>8)&0xff), rfpga->regbase+REEL_CI_CA); // addr high
	writel(data, rfpga->regbase+REEL_CI_DATA);
	writel(REEL_CI_OE|REEL_CI_CES|REEL_CI_BUFD|ce_tab_write[slot]|0x80, rfpga->regbase+REEL_CI_CA); // CE+BUFDIR
	writel(REEL_CI_OE|REEL_CI_CTRL|REEL_CI_BUFD|ctrl_val, rfpga->regbase+REEL_CI_CA); // WE
	writel(REEL_CI_OE|REEL_CI_CTRL|REEL_CI_BUFD|REEL_CI_OEWR|ctrl_val, rfpga->regbase+REEL_CI_CA); 
	writel(REEL_CI_OE|REEL_CI_CES|0xff, rfpga->regbase+REEL_CI_CA); 
	spin_unlock_irqrestore(&rfpga->lock, flags);

	up(&rfpga->ci_mutex);
	return 0;
}

/* ------------------------------------------------------------------------- */
int reel_ci_read_attribute_mem(struct dvb_ca_en50221* ca, int slot, int addr) 
{
	struct reel_unit *ru=(struct reel_unit*) ca->data;
	struct reelfpga *rfpga=ru->rfpga;

	return reel_ci_read(rfpga, slot, addr&0x7fff);
}
/* ------------------------------------------------------------------------- */
static int reel_ci_write_attribute_mem(struct dvb_ca_en50221* ca, int slot, int addr, u8 value) 
{
	struct reel_unit *ru=(struct reel_unit*) ca->data;
	struct reelfpga *rfpga=ru->rfpga;
	return reel_ci_write(rfpga, slot, (addr&0x7fff), value);
}
/* ------------------------------------------------------------------------- */
static int reel_ci_read_cam_control(struct dvb_ca_en50221* ca, int slot, u8 addr) 
{
	struct reel_unit *ru=(struct reel_unit*) ca->data;
	struct reelfpga *rfpga=ru->rfpga;
	return reel_ci_read(rfpga, slot, (addr&0x7fff)|(1<<15));
}
/* ------------------------------------------------------------------------- */
static int reel_ci_write_cam_control(struct dvb_ca_en50221* ca, int slot, u8 addr, u8 value) 
{
	struct reel_unit *ru=(struct reel_unit*) ca->data;
	struct reelfpga *rfpga=ru->rfpga;
	return reel_ci_write(rfpga, slot, (addr&0x7fff)|(1<<15), value);
}
/* ------------------------------------------------------------------------- */
int reel_ci_reset(struct reelfpga *rfpga, int slot)
{
	int val,val1;
	if (slot<0 || slot>2)
		return -EINVAL;

	printk("CI Slot reset %i\n",slot);
	down(&rfpga->ci_mutex);
	writel(REEL_CI_OE|REEL_CI_CES|0xff, rfpga->regbase+REEL_CI_CA); 
	val=rfpga->ci_resetstate&(~reset_tab[slot]);
	val1=REEL_CI_OE|REEL_CI_CTRL|rfpga->ci_resetstate|reset_tab[slot]|REEL_CI_OEWR;
	writel(val1, rfpga->regbase+REEL_CI_CA); 
	msleep(100);
	writel(REEL_CI_OE|REEL_CI_CTRL|REEL_CI_OEWR|val, rfpga->regbase+REEL_CI_CA);
	rfpga->ci_resetstate=val;
	up(&rfpga->ci_mutex);
	msleep(1000);
	return 0;
}
/* ------------------------------------------------------------------------- */
static void reel_ci_matrix_route(struct reelfpga *rfpga)
{
	int n,last=-1,v,unit;
	int hw_unit;
	int src_tab[6]={REEL_SRC_CI(0),REEL_SRC_CI(1),REEL_SRC_CI(2),
				REEL_SRC_X(0),REEL_SRC_X(1),REEL_SRC_X(2)};
				
	int dst_tab[6]={REEL_DST_CI(0),REEL_DST_CI(1),REEL_DST_CI(2),
				REEL_DST_X(0),REEL_DST_X(1),REEL_DST_X(2)};

	v=0;
	for(n=0;n<3;n++) {
		if (rfpga->ci_assoc[n]!=-1)
			v|=(1<<n);
	}
	writel(v, rfpga->regbase+REEL_CI_ENABLE);

	for(unit=0;unit<rfpga->connected_units;unit++) {
		hw_unit=rfpga->reels[unit]->hw_unit;
		last=-1;
		for(n=0;n<6;n++) {
			if (rfpga->ci_assoc[n]==unit) {
				if (last==-1)
					set_ts_matrix(rfpga, REEL_SRC_TS(hw_unit) , dst_tab[n]);
				else
					set_ts_matrix(rfpga, src_tab[last], dst_tab[n]);
				last=n;
			}
		}
		if (last==-1)
			set_ts_matrix(rfpga, REEL_SRC_TS(hw_unit) , REEL_DST_PIDF(unit));
		else
			set_ts_matrix(rfpga, src_tab[last], REEL_DST_PIDF(unit));
		
		set_ts_matrix(rfpga, REEL_SRC_PIDF(unit), REEL_DST_DMA(unit));
	}
}
/* ------------------------------------------------------------------------- */
// Returns Card Detect CD1 in bit 3,5,7 and CD2 in 4,6,8, and Voltage State VS1 in 12-14
// Needs lock (ci_mutex)!
static int reel_ci_get_cdvs(struct reelfpga *rfpga)
{
	int ret1, ret2;
	writel(REEL_CI_OE|REEL_CI_BUF1,rfpga->regbase+REEL_CI_CA);
	ret1=readl(rfpga->regbase+REEL_CI_CA);
	
	writel(REEL_CI_OE|REEL_CI_BUF2,rfpga->regbase+REEL_CI_CA);
	ret2=readl(rfpga->regbase+REEL_CI_CA);
	
	ret1=(ret1&255)|(ret2<<8);
//	printk("%x\n",ret1);
	return ret1;
}
/* ------------------------------------------------------------------------- */
//                           REELFPGA IOCTLS
/* ------------------------------------------------------------------------- */

// Returns card detect in 0-2, change status in 8-10, and power state in 16-18
// Clears change state when arg!=0
int reel_ci_get_state(struct reelfpga *rfpga, int arg)
{
	int ret;

	down(&rfpga->ci_mutex);
	ret=rfpga->ci_slotstate|(rfpga->ci_changed<<8);
	ret|=(rfpga->ci_real_powerstate[0]<<16)|
		(rfpga->ci_real_powerstate[1]<<18)|
		(rfpga->ci_real_powerstate[2]<<20);
	if (arg)
		rfpga->ci_changed=0;
	up(&rfpga->ci_mutex);

	return ret;		
}
/* ------------------------------------------------------------------------- */
// Power on individual CI slot

int reel_ci_switch(struct reelfpga *rfpga, int slot, int state)
{       
	if (slot>2 || slot<0)
		return -EINVAL;

	state=state&3;

	if (state) {
		if (rfpga->ci_slotstate&(1<<slot)) {
			reel_ci_power(rfpga, slot, 5);			// Power on, FIXME
			msleep(100);
			if (state&2)
				rfpga->ca_slot_enable|=1<<slot;        	// Enable for DVB CA core
			else
				rfpga->ca_slot_enable&=~(1<<slot);      // Hidden from DVB CA core
			rfpga->ci_real_powerstate[slot]=state;       	
			msleep(100);
		}
	}
	else {
		
		rfpga->ca_slot_enable&=~(1<<slot);	       	// Remove from DVB CA core
		rfpga->ci_assoc[slot]=-1;                       // Remove from Matrix
		reel_ci_matrix_route(rfpga);
		reel_ci_power(rfpga, slot, 0);			// Power off
		rfpga->ci_real_powerstate[slot]=state;       	
	}

	return 0;
}
/* ------------------------------------------------------------------------- */
// Slot: CAM-Slot (0..2/0xff), unit: Tuner (0..3/0xff)
int reel_ci_change_assoc(struct reelfpga *rfpga, int slot, int unit)
{
	int set_matrix=0;

	slot&=0xff;
	if (slot>5 && slot!=0xff)
		return -EINVAL;

	unit&=0xff;

	if (unit>3 && unit!=0xff)
		return -EINVAL;

	down(&rfpga->ci_mutex);

	if (slot==0xff) { // only Tuner-unit given, release associated CAM(s) (if any)
		int n;
		for(n=0;n<6;n++)
			if (rfpga->ci_assoc[n]==unit) {
				rfpga->ci_assoc[n]=0xff;
				set_matrix=1;
			}
	}
	else if (rfpga->ci_assoc[slot]!=unit) {  // CAM given, set (0..3) or release (0xff) tuner unit
		rfpga->ci_assoc[slot]=unit;
		set_matrix=1;
	}

	if (set_matrix)
		reel_ci_matrix_route(rfpga);

	up(&rfpga->ci_mutex);
	return 0;
}
/* ------------------------------------------------------------------------- */
// Actual slot detection, needs ci_mutex

static int reel_ci_detect_slots(struct reelfpga *rfpga)
{
	int ret1,ret2;
	int i,slotstate;
	int old_state=rfpga->ci_slotstate;
	int changestate;

	ret1=reel_ci_get_cdvs(rfpga);
	ret2=ret1;
	ret1|=rfpga->ci_delay;
	rfpga->ci_delay=ret2;
	slotstate=0;
	for(i=0;i<3;i++) {
		if ((ret1&(3<<(3+2*i)))==0)
			slotstate|=1<<i;
	}
	rfpga->ci_slotstate=slotstate;
	changestate=slotstate^old_state;
//	printk("STATE %x %x\n",slotstate, changestate);
	return changestate;
}
/* ------------------------------------------------------------------------- */
//                        DVB CA CORE METHODS
/* ------------------------------------------------------------------------- */

// Note: RESET should be applied anyway after plug-in until reel_ci_slot_reset is issued

int reel_ci_slot_reset(struct dvb_ca_en50221* ca, int slot) 
{
	struct reel_unit *ru=(struct reel_unit*) ca->data;
	struct reelfpga *rfpga=ru->rfpga;
	
	return reel_ci_reset(rfpga, slot);
}

/* ------------------------------------------------------------------------- */
static int reel_ci_slot_shutdown(struct dvb_ca_en50221* ca, int slot) 
{
	struct reel_unit *ru=(struct reel_unit*) ca->data;
	struct reelfpga *rfpga=ru->rfpga;
	int val;


	if (slot<0 || slot>2)
		return -EINVAL;

	printk("reel_ci_slot_shutdown for slot %i\n",slot);

	if (!rfpga->ci_real_powerstate[slot])
		return 0;

	down(&rfpga->ci_mutex);

	rfpga->ci_assoc[slot]=-1;
	reel_ci_matrix_route(rfpga);		

	val=rfpga->ci_resetstate| reset_tab[slot];

	writel(REEL_CI_OE|REEL_CI_CTRL|val|REEL_CI_OEWR,rfpga->regbase+REEL_CI_CA);

	rfpga->ci_resetstate=val;
	up(&rfpga->ci_mutex);

//	reel_ci_power(rfpga, slot, 0);
	rfpga->ci_real_powerstate[slot]=0;

        return 0;
}
/* ------------------------------------------------------------------------- */
static int reel_ci_slot_ts_enable(struct dvb_ca_en50221* ca, int slot) 
{
//	struct reel_unit *ru=(struct reel_unit*) ca->data;
//	struct reelfpga *rfpga=ru->rfpga;

	if (slot<0 || slot>2)
		return -EINVAL;
	printk("reel_ci_slot_ts_enable for slot %i\n",slot);
	// All routing done via userspace...

	return 0;
}
/* ------------------------------------------------------------------------- */
// Abused for change detection for get_state-ioctl
static int reel_ci_slot_status(struct dvb_ca_en50221* ca, int slot, int open)
{
	struct reel_unit *ru=(struct reel_unit*) ca->data;
	struct reelfpga *rfpga=ru->rfpga;
	int slotstate,detect=0;
	int retval=0,changed;
	int use_slot;
	
	if (slot<0 || slot>2)
	        return -EINVAL;

	down(&rfpga->ci_mutex);

	rfpga->ci_changed|=reel_ci_detect_slots(rfpga);
	slotstate=rfpga->ci_slotstate;

	up(&rfpga->ci_mutex);

	slotstate&=rfpga->ca_slot_enable;  // allow core handling?

	if ( slotstate&(1<<slot))
		detect=1;
	
	changed=rfpga->ca_old_state[slot]^detect;

	rfpga->ca_old_state[slot]=detect;

	if (detect) {
//		printk("CI%i detected for RU%i\n",slot,ru->num);
		retval=DVB_CA_EN50221_POLL_CAM_PRESENT|DVB_CA_EN50221_POLL_CAM_READY;
	}

	if (changed) {
		printk("CI slot %i changed for Reel %i, now: %i, enable %i\n", slot, ru->num, detect, rfpga->ca_slot_enable);
		retval|=DVB_CA_EN50221_POLL_CAM_CHANGED;
	}

	up(&rfpga->ci_mutex);

	use_slot=rfpga->ca_slot_enable&(1<<slot);
	if (changed && use_slot ) {

		if (!detect) 
			reel_ci_power(rfpga, slot, 0);	// Auto off	 
		else {
#ifdef CORE_HANDLING		
			reel_ci_power(rfpga, slot, 5);  // Auto power on
			msleep(400);
#endif
                }
	}
	return retval;
}
/* ------------------------------------------------------------------------- */
void reel_ci_init(struct reelfpga *rfpga)
{
	int n;

	//writel( 0x00, rfpga->regbase+REEL_CI_ENABLE); // disable all TS output
	writel( 0x07, rfpga->regbase+REEL_CI_ENABLE); // TB

	// set pins
	writel(REEL_CI_CES|0xff, rfpga->regbase+REEL_CI_CA);
	writel(REEL_CI_LOW_ADDR, rfpga->regbase+REEL_CI_CA);
	writel(REEL_CI_HIGH_ADDR, rfpga->regbase+REEL_CI_CA);
	writel(REEL_CI_OE|REEL_CI_CTRL|REEL_CI_OEWR, rfpga->regbase+REEL_CI_CA);
	
	for(n=0;n<3;n++) {
		reel_ci_power(rfpga, n, 0);
		rfpga->ci_real_powerstate[n]=0;
		rfpga->ca_old_state[n]=0;
	}
		
	rfpga->ci_resetstate=reset_tab[0]|reset_tab[1]|reset_tab[2];
	writel(rfpga->ci_resetstate|REEL_CI_OE|REEL_CI_CTRL|REEL_CI_OEWR,rfpga->regbase+REEL_CI_CA);
	rfpga->ci_powerstate=0;
	rfpga->ci_slotstate=0;
	rfpga->ci_changed=0;

	rfpga->ca_slotstate=0;
#ifdef CORE_HANDLING
	rfpga->ca_slot_enable=CORE_SLOTS;
#else
	rfpga->ca_slot_enable=0;
#endif
	rfpga->ci_delay=0xffff;

	for(n=0;n<6;n++) {
		rfpga->ci_assoc[n]=255;
	}
}
/* ------------------------------------------------------------------------- */
void reel_ci_attach(struct reel_unit *ru)
{
	int result;

	memset(&ru->ca, 0, sizeof(struct dvb_ca_en50221));
	ru->ca.read_attribute_mem=reel_ci_read_attribute_mem;
	ru->ca.write_attribute_mem=reel_ci_write_attribute_mem;
	ru->ca.read_cam_control=reel_ci_read_cam_control;
	ru->ca.write_cam_control=reel_ci_write_cam_control;
	ru->ca.slot_reset=reel_ci_slot_reset;
	ru->ca.slot_shutdown=reel_ci_slot_shutdown;
	ru->ca.slot_ts_enable=reel_ci_slot_ts_enable;
	ru->ca.poll_slot_status=reel_ci_slot_status;
	ru->ca.data=ru;	

	if ((result = dvb_ca_en50221_init(&ru->dvb_adapter,
                                          &ru->ca,
					  /*
                                          DVB_CA_EN50221_FLAG_IRQ_CAMCHANGE |
                                          DVB_CA_EN50221_FLAG_IRQ_FR |
                                          DVB_CA_EN50221_FLAG_IRQ_DA*/
					  0,
					  3)) != 0) {
                printk("reeldvb: CI initialisation failed.\n");
                return;
        }
	ru->ca_used=1;
	printk("CI init done for Reel %i\n",ru->num);
}
/* ------------------------------------------------------------------------- */
void reel_ci_detach(struct reel_unit *ru)
{
	dvb_ca_en50221_release(&ru->ca);
}
