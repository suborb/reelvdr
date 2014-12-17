/*****************************************************************************/
/*
 * DMA and other HW functions for Reelbox FPGA (4 tuner version)
 *
 * Copyright (C) 2004-2005 Georg Acher, BayCom GmbH (acher at baycom dot de)
 *
 *
 * #include <GPL-header>
 *
 * $Id: reel_hw.c,v 1.2 2005/09/10 14:34:16 acher Exp $
 *
 */    
/*****************************************************************************/

#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/timer.h>
#include <linux/sched.h>
#include <linux/smp_lock.h>
#include <linux/module.h>
#include <linux/list.h>
#include <linux/vmalloc.h>
#include <linux/bitops.h>
#include <asm/uaccess.h>
#include <asm/io.h>
#include <linux/pci.h>
#include <linux/device.h>
#include <linux/mm.h>
#include <linux/interrupt.h>

#include <linux/delay.h>
#include <linux/dma-mapping.h>

// FIXME shorten includes a bit...

#include "reeldvb.h"

#define dbg(format, arg...) printk(format"\n", ## arg)
#define err(format, arg...) printk(format"\n", ## arg)

/* ------------------------------------------------------------------------- */
// Low level DMA access
/* ------------------------------------------------------------------------- */

int dma_setup(struct reelfpga *s, int channel, int subslot, dma_addr_t addr, int len)
{
	if (channel<0 || channel>3)
		return -1;
	if (subslot<0 || subslot>3)
		return -1;
//	printk("DMA%i.%i: %x %x\n",channel,subslot,addr,len);
	writel(addr, s->regbase + REEL_DMA_ADDR00 + 8*subslot + 32*channel);
	writel(len,  s->regbase + REEL_DMA_LEN00  + 8*subslot + 32*channel);
	return 0;
}
/* ------------------------------------------------------------------------- */
int dma_control(struct reelfpga *s, int channel, int cmd)
{
	if (channel<0 || channel>3)
		return -1;
	
	writel(cmd, s->regbase+REEL_DMA0+4*channel);
	return 0;
}
/* ------------------------------------------------------------------------- */
//
// High level DMA buffer management
//
/* ------------------------------------------------------------------------- */
void start_subslot(struct reel_channel *rct, int slot, int subslot, int latency_fix)
{
	int small_len;

	if (rct->state==0 || latency_fix) { 
		if (latency_fix==2)
			small_len=BUF_SIZE_S;
		else
			small_len=BUF_SIZE_M;
		// small buffer for faster startup sync
		rct->rec[slot]->len[subslot]=small_len;
		dma_setup(rct->reelfpga, rct->channel, subslot, 
			  rct->rec[slot]->dma[subslot], rct->rec[slot]->len[subslot]);
	}
	else {
		rct->rec[slot]->len[subslot]=rct->rec[slot]->len_alloc[subslot];
		dma_setup(rct->reelfpga, rct->channel, subslot, 
			  rct->rec[slot]->dma[subslot], rct->rec[slot]->len[subslot]);
	}
}
/* ------------------------------------------------------------------------- */
void start_dma(struct reel_channel *rct, int slot, int latency_fix)
{
	int subslot;

	for(subslot=0;subslot<4;subslot++) {
#if 0
		printk("DMA Setup %i.%i.%i: %x bytes @ %x\n", rct->channel, slot, subslot, 
		       rct->rec[slot]->len[subslot], rct->rec[slot]->dma[subslot] );
#endif

		start_subslot(rct,slot,subslot,latency_fix);
	}

	// This check detects only delays in dvbcore handling
	if (rct->rec[slot]->finished[0]||rct->rec[slot]->finished[1]||
	    rct->rec[slot]->finished[2]||rct->rec[slot]->finished[3]) 
	{
		rct->overflows++;
		err("DMA %i.%i: overflows %i, now %li",
		    rct->channel,slot,rct->overflows,jiffies);	
	}

	rct->rec[slot]->finished[0]=0;
	rct->rec[slot]->finished[1]=0;
	rct->rec[slot]->finished[2]=0;
	rct->rec[slot]->finished[3]=0;

	dma_control(rct->reelfpga, rct->channel, rct->flags);
//	printk("DMA %i.%i: start @ %p %p\n",rct->channel,slot,rct->rec[slot]->mem[0],rct->rec[slot]->mem[1]);
}
/* ------------------------------------------------------------------------- */
void stop_dma(struct reel_channel *rct)
{
	rct->flags=0;
	dma_control(rct->reelfpga, rct->channel, 0);  // full stop, fifo clear
}
/* ------------------------------------------------------------------------- */
struct reel_dma *allocate_dma_descr(struct reelfpga *s, int full_buf_size)
{
	char* mem;
	dma_addr_t dma;
	struct reel_dma *rec;
	int n;

	rec=(struct reel_dma*)kmalloc(sizeof(struct reel_dma),GFP_KERNEL);

	if (!rec)
		goto fail;

	memset(rec,0,sizeof(struct reel_dma));

	for(n=0;n<4;n++) {
		mem=pci_alloc_consistent (s->pci_dev, full_buf_size, &dma);
		if (!mem) {
			err("Failed to get DMA mem (%i bytes)",BUF_SIZE);
			goto fail;
		}

		rec->mem[n]=mem;
		rec->len_alloc[n]=full_buf_size;
		rec->dma[n]=dma;
	}

	return rec;
fail:	
	free_dma_descr(s,rec);
	return NULL;
}
/* ------------------------------------------------------------------------- */
void free_dma_descr(struct reelfpga *s, struct reel_dma *rec)
{
	int n;
	if (rec) {
		for(n=0;n<4;n++)
			if (rec->mem[n])
				pci_free_consistent (s->pci_dev, rec->len_alloc[n] , 
						     rec->mem[n], rec->dma[n]);
		kfree(rec);
	}
}
/* ------------------------------------------------------------------------- */
void init_dma_channel(struct reel_channel *rct)
{
	int n;
	for(n=0;n<BUF_NUM;n++) {
		rct->rec[n]->finished[0]=0;
		rct->rec[n]->finished[1]=0;
		rct->rec[n]->finished[2]=0;
		rct->rec[n]->finished[3]=0;
	}
	rct->pos=0;
	rct->current_slot=0;
	rct->current_subslot=0;
	rct->read_slot=0;
	rct->read_subslot=0;
	rct->flags=0;
	rct->flags1=0;
	rct->overflows=0;
	rct->state=0;
}
/* ------------------------------------------------------------------------- */
struct reel_channel *allocate_dma_channel(struct reelfpga *s, int channel)
{
	struct reel_channel *rct;
	int n;

	rct=(struct reel_channel*)kmalloc(sizeof(struct reel_channel),GFP_KERNEL);
	if (!rct)
		return NULL;
	rct->channel=channel;
	for(n=0;n<BUF_NUM;n++) 
		rct->rec[n]=NULL;

	for(n=0;n<BUF_NUM;n++) {
		rct->rec[n]=allocate_dma_descr(s,BUF_SIZE);
		if (!rct->rec[n])
			goto fail;
	}

	init_dma_channel(rct);
	init_waitqueue_head (&rct->wait);
	rct->reelfpga=s;
//	dbg("Allocated DMA%i, %p ",channel, rct);
	return rct;

fail:
	if (rct) {
		for(n=0;n<BUF_NUM;n++) {
			if (rct->rec[n])
				free_dma_descr(s,rct->rec[n]);
		}
		kfree(rct);
	}
	
	return NULL;
}
/* ------------------------------------------------------------------------- */
void free_dma_channel(struct reel_channel *rct)
{	
	int n;
	if (rct) {
		stop_dma(rct);
		for(n=0;n<BUF_NUM;n++) {
			if (rct->rec[n])
				free_dma_descr(rct->reelfpga,rct->rec[n]);
		}
		kfree(rct);
	}	
}
/* ------------------------------------------------------------------------- */
void init_pidfilter(struct reelfpga *s, int num)
{
	int n;
	
	switch (num) {
	case 0:

		for(n=0;n<8192/8;n+=4)
			writel(0x0,s->regbase+REEL_PIDF0BASE+REEL_PIDF_RAM+n);
		writel( 0 , s->regbase+REEL_PIDF0BASE+REEL_PIDF_CTRL);
		break;

	case 1:
		for(n=0;n<8192/8;n+=4)
			writel(0x0,s->regbase+REEL_PIDF1BASE+REEL_PIDF_RAM+n);
		writel( 0 , s->regbase+REEL_PIDF1BASE+REEL_PIDF_CTRL);
		break;
	case 2:
		for(n=0;n<6;n++) {
			writel(0x0,s->regbase+REEL_PIDF2BASE+REEL_PIDF23_RAMA+n*4);
			writel(0x0,s->regbase+REEL_PIDF2BASE+REEL_PIDF23_RAMB+n*4);
		}
		writel( 0 , s->regbase+REEL_PIDF2BASE+REEL_PIDF_CTRL);
		break;
	case 3:
		for(n=0;n<6;n++) {
			writel(0x0,s->regbase+REEL_PIDF3BASE+REEL_PIDF23_RAMA+n*4);
			writel(0x0,s->regbase+REEL_PIDF3BASE+REEL_PIDF23_RAMB+n*4);
		}
		writel( 0 , s->regbase+REEL_PIDF3BASE+REEL_PIDF_CTRL);
		break;
	}
}
/* ------------------------------------------------------------------------- */
// mode: 0: clear PID, 1: set PID
// PID=0x2000 -> feed through
// return 0: OK
int set_pidfilter(struct reel_unit *ru, int mode, int pid)
{
	void *base;
	int v,n,m,v1;
	int mask,slot;
	unsigned long flags;
	int ret=0;
	struct reelfpga* s=ru->rfpga;
	spin_lock_irqsave(&s->hw_lock,flags);

	if (ru->num==0 || ru->num==1) { // Complex PID filters
//		printk("Set FULL PIDF %i, %i to %i\n",ru->num,pid,mode);
		base=s->regbase+(ru->num==0?REEL_PIDF0BASE:REEL_PIDF1BASE);
		if (pid==0x2000) {
			if (mode==1)
				writel( REEL_PIDF_BYPASS , base+REEL_PIDF_CTRL);
			else
				writel( 0 , base+REEL_PIDF_CTRL);
		}
		else {
			slot=4*(pid>>5);
			mask=1<<(pid&31);
			v=readl(base+REEL_PIDF_RAM+slot);
			if (mode==0)
				v1=v&(~mask);			
			else
				v1=v|mask;
			if (v!=v1) {
//				printk("PID: (%x) %x %x\n",slot,v,v1);
				writel(v1,base+REEL_PIDF_RAM+slot);
			}
		}
	}
	else {			
		// small PID filters (up to 2*12 PIDs)
		base=s->regbase+(ru->num==2?REEL_PIDF2BASE:REEL_PIDF3BASE);
//		printk("Set SIMPLE PIDF %i, %i to %i\n",ru->num,pid,mode);
		if (mode==0) { // Clear PID
			for(m=0;m<2;m++) {
				for(n=0;n<6;n++) {
					void* base1=base+REEL_PIDF23_RAMA+4*n;

					if (m==1)
						base1=base+REEL_PIDF23_RAMB+4*n;

					v=readl(base1);
					if ((v&0x3fff) == (8192|pid)) {
						writel(v&0xffff0000,base1);
						goto out;
					}
					if (((v>>16)&0x3fff) == (8192|pid)) {
						writel(v&0xffff,base1);
						goto out;
					}
				}				
			}
		}
		else {  // Set PID
			for(m=0;m<2;m++) {

				for(n=0;n<6;n++) {
					void* base1=base+REEL_PIDF23_RAMA+4*n;

					if (m==1)
						base1=base+REEL_PIDF23_RAMB+4*n;

					v=readl(base1);
					if (!(v&8192)) {
						writel((v&0xffff0000)|(8192|pid),base1);
						goto out;
					}
					if (!(v&0x20000000)) {
						writel((v&0xffff)|((8192|pid)<<16),base1);
						goto out;
					}
				}
			}

			ret=-1; // out of PIDs
			printk("Set PIDF %i: Out of PIDs\n",ru->num);
		}
	}
out:
	spin_unlock_irqrestore(&s->hw_lock,flags);
	return ret;
}
/* ------------------------------------------------------------------------- */
void set_ts_matrix(struct reelfpga* rfpga, int source, int destination)
{	
	char *src_tab[]={"TS0","PIDF0","TS1","PIDF1","TS2","PIDF2","TS3","PIDF3",
			 "CI0","X0","CI1","X1","CI2","X2"};
	char *dst_tab[]={"DMA0","PIDF0","DMA1","PIDF1","DMA2","PIDF2","DMA3","PIDF3",
			 "CI0","X0","CI1","X1","CI2","X2"};
	if (source <0 || source>13 || destination<0 || destination >13)
		return;
	printk("Matrix: %s -> %s\n",src_tab[source],dst_tab[destination]);
	writel(source, rfpga->regbase+REEL_MATRIXBASE + 4*destination);
}
/* ------------------------------------------------------------------------- */
void init_reel_hw(struct reelfpga* s)
{
	int n;

	writel(255,s->regbase+REEL_DMA_IRQSTATUS); // Clear pending IRQs

	for(n=0;n<4;n++) 
		init_pidfilter(s,n);

	// Overwritte by tuner detection
#if 1
	// TS0->PIDF0->DMA0
	set_ts_matrix(s, REEL_SRC_TS0, REEL_DST_PIDF0);
	set_ts_matrix(s, REEL_SRC_PIDF0, REEL_DST_DMA0);

	// TS1->PIDF1->DMA1
	set_ts_matrix(s, REEL_SRC_TS1, REEL_DST_PIDF1);
	set_ts_matrix(s, REEL_SRC_PIDF1, REEL_DST_DMA1);

	// TS2->PIDF2->DMA2
	set_ts_matrix(s, REEL_SRC_TS2, REEL_DST_PIDF2);
	set_ts_matrix(s, REEL_SRC_PIDF2, REEL_DST_DMA2);

	// TS3->PIDF3->DMA3
	set_ts_matrix(s, REEL_SRC_TS3, REEL_DST_PIDF3);
	set_ts_matrix(s, REEL_SRC_PIDF3, REEL_DST_DMA3);
#else
	set_ts_matrix(s, REEL_SRC_TS0, REEL_DST_DMA0);
	set_ts_matrix(s, REEL_SRC_TS1, REEL_DST_DMA1);
	set_ts_matrix(s, REEL_SRC_TS2, REEL_DST_DMA2);
	set_ts_matrix(s, REEL_SRC_TS3, REEL_DST_DMA3);

#endif

}
/* ------------------------------------------------------------------------- */
int reel_push_ts(struct reelfpga* rfpga, int count, unsigned char *buf)
{
	int n,m;
	volatile short *x1,*x2;

	x1=(volatile short*)(rfpga->regbase+REEL_X0_BASE+8);
	x2=(volatile short*)(rfpga->regbase+REEL_X0_BASE);

	for(n=0;n<count;n++) {
		*x1=*(short*)buf;
		buf+=2;
		*x2=*(short*)buf;
		buf+=2;
		for(m=4;m<188;m+=4) {
			*x2=*(short*)buf;
			buf+=2;
			*x2=*(short*)buf;
			buf+=2;
		}
	}
	return 0;
}
/* ------------------------------------------------------------------------- */
// Control Tuner modules
int reel_gpio_idreg(struct reelfpga* rfpga, int slot, int val)
{
	int a=readl(rfpga->regbase+REEL_GPIO_IDOUT);
	if (slot==0)
		a=(a&0xff00)|(val&0xff);
	else
		a=(a&0xff)|((val&0xff)<<8);
	writel(a,rfpga->regbase+REEL_GPIO_IDOUT);
	return 0;
}
/* ------------------------------------------------------------------------- */
// Has nothing to do with FPGA, but it has to be done somewhere...
void set_isa_demux(int value)
{
	unsigned long flags;
	local_irq_save(flags);
	udelay(2);
	outl(inl(0x9034) | 0x0100, 0x9034);
	outw(value, 0x340);
	outl(inl(0x9034) & 0xfeff, 0x9034);
	udelay(2);
	local_irq_restore(flags);
}

/* ------------------------------------------------------------------------- */
