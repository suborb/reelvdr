/*****************************************************************************/
/*
 * DVB-API driver for Reelbox FPGA (4 tuner version)
 *
 * Copyright (C) 2004-2005  Georg Acher, BayCom GmbH (acher at baycom dot de)
 *
 * Based on budget-core (C) 1999-2002 Ralph Metzler & Marcus Metzler
 *                               (rjkm at metzlerbros dot de)
 *
 * #include <GPL-header>
 *
 *  $Id: reel_core.c,v 1.3 2005/09/24 18:41:17 acher Exp $
 *
 */       
/*****************************************************************************/

#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/timer.h>
#include <linux/sched.h>
#include <linux/devfs_fs_kernel.h>
#include <linux/smp_lock.h>
#include <linux/module.h>
#include <linux/list.h>
#include <linux/vmalloc.h>
#include <linux/init.h>
#include <linux/poll.h>
#include <linux/bitops.h>
#include <asm/uaccess.h>
#include <asm/io.h>
#include <linux/pci.h>
#include <linux/device.h>
#include <linux/mm.h>
#include <linux/interrupt.h>
#include <linux/wait.h>

#include <linux/miscdevice.h>
#include <linux/delay.h>
#include <asm/scatterlist.h>
#include <linux/dma-mapping.h>

#include "reeldvb.h"

#define dbg(format, arg...) printk(format"\n", ## arg)
#define err(format, arg...) printk(format"\n", ## arg)

static DECLARE_COMPLETION(thread_exited);


/* --------------------------------------------------------------------- */
int reel_start_feed(struct dvb_demux_feed *feed)
{
        struct dvb_demux *demux = feed->demux;
        struct reel_unit *ru = (struct reel_unit*) demux->priv;

        if (!demux->dmx.frontend)
                return -EINVAL;

	if (feed->pid >0x2000)
	 	return -EINVAL;

//	printk("DMA %i: Start feed #%i for PID %i %i\n", ru->num, ru->feeding+1, feed->pid,feed->type);
	
	if (!ru->feeding && ru->num>1)
		init_pidfilter(ru->rfpga, ru->num);

	if (set_pidfilter(ru, 1, feed->pid)) {
		printk("DMA %i: Failed feed #%i for PID %i %i\n", ru->num, ru->feeding+1, feed->pid,feed->type);
	 	return -ENOSPC; // Too much PIDs required (can only happen with dma2/3)
	}

	if (ru->feeding) {
		ru->feeding++;
                return ru->feeding; // No DMA startup, already running
	}

	printk("DMA %i: Fresh start\n", ru->num);
	init_dma_channel(ru->rct);

	ru->rct->state=0; // small buffer at the beginning

	ru->rct->flags=0;
	start_dma(ru->rct, 0, 1);
	udelay(5); // This precharge time should be fixed in FPGA. Someday :-)

	ru->rct->flags = REEL_DMA_ENABLED|REEL_DMA_IOC|REEL_DMA_CONT|REEL_DMA_SYNC|REEL_DMA_VALID;
	start_dma(ru->rct, 0, 1);

	ru->rct->state=1; // large buffer for the rest of time

	ru->feeding++;

	ru->last_transfer_jiffies=jiffies;
	ru->latency_fix=1;

	return ru->feeding;
}
/* --------------------------------------------------------------------- */
static int reel_stop_feed(struct dvb_demux_feed *feed)
{
        struct dvb_demux *demux = feed->demux;
        struct reel_unit *ru = (struct reel_unit*) demux->priv;

// 	printk("DMA %i: Stop feed #%i for PID %i\n",ru->num,ru->feeding, feed->pid);

	set_pidfilter(ru, 0, feed->pid); // remove PID from filter

	ru->feeding--;

        if (ru->feeding)
                return ru->feeding;
	ru->rct->flags=0;
	ru->rct->state=0;
	printk("DMA %i: Full stop\n", ru->num);
	stop_dma(ru->rct);
	
        return 0;
}
/* --------------------------------------------------------------------- */
#if 0
void reel_restart_feed(struct reel_unit *ru)
{
	printk("DMA %i: Restart DMA\n",ru->num);
	ru->rct->flags=0;
	ru->rct->state=0;
	printk("DMA %i: Full stop\n", ru->num);
	stop_dma(ru->rct);

	printk("DMA %i: Fresh start\n", ru->num);
	init_dma_channel(ru->rct);
	ru->rct->state=0; // small buffer at the beginning

	ru->rct->flags=0;
	start_dma(ru->rct, 0);
	udelay(5); // This precharge time should be fixed in FPGA. Someday :-)

	ru->rct->flags = REEL_DMA_ENABLED|REEL_DMA_IOC|REEL_DMA_CONT|REEL_DMA_SYNC|REEL_DMA_VALID;
	start_dma(ru->rct, 0);

	ru->rct->state=1; // large buffer for the rest of time
}
#endif
/* --------------------------------------------------------------------- */
int lco=0;
void check_con(int num, int slot, int subslot,unsigned char *buffer, int len)
{
	int n,pidc=0x65;

	for(n=0;n<len;n+=188) {
		int pid;
		int co;
		
		pid=((buffer[n+1]&15)<<8)+buffer[n+2];
		if (buffer[n+3]&0x10 && pid==pidc) {
                        co=buffer[n+3]&15;
			
                        if (co!=((lco+1)&15)) 
                        {
                                printk("XX %i.%i.%i %i %x now: %x last: %x\n",num,slot,subslot,n,pid,co,lco);
//                                dump_hex(&buffer[n],188);
//                                dump_hex(&buffer[n-188],32);
//                                errs++;
                        }
                        lco=co;
		}
	}

}
/* --------------------------------------------------------------------- */
// Transfer data from DMA-buffers to DVB core
void reel_transfer_data(struct reelfpga *s)
{
	struct reel_unit *ru;
	struct reel_channel *rct;
	struct reel_dma *rec;
	unsigned long flags;
	int n,read_slot,read_subslot;
	int current_slot,current_subslot;
	unsigned char *mem;
	ssize_t len;

	for(n=0;n<4;n++) {

		ru=s->reels[n];

		if (ru && ru->feeding && ru->rct) {				
			rct=ru->rct;

			while(1) {
				spin_lock_irqsave(&s->lock, flags);
				read_slot=rct->read_slot;
				read_subslot=rct->read_subslot;
				current_slot=rct->current_slot;
				current_subslot=rct->current_subslot;
								
				if (read_slot==current_slot&& read_subslot==current_subslot ) {
					spin_unlock_irqrestore(&s->lock, flags);
					break;
				}
				
				rec=rct->rec[read_slot];
				mem=rec->mem[read_subslot];
				len=rec->len[read_subslot];
				spin_unlock_irqrestore(&s->lock, flags);
#if 0
//				printk("Fin %i.%i.%i @ %p %i\n",ru->num,
//				       read_slot,read_subslot,mem,len);
				for(m=0;m<6;m++) {
					printk("%02x: ",m*32);
					for(n=0;n<32;n++)
						printk("%02x ",*(mem+n+32*m));
					printk("\n");
				}
#endif

//				check_con(ru->num,read_slot,read_subslot,mem,len);

//				printk ("%p %02x %02x %02x %02x\n",mem,mem[0],mem[1],mem[2],mem[3]);
				dvb_dmx_swfilter_packets(&ru->demux, mem, len/188);
//				memset(mem,0xff,len);

				rec->finished[read_subslot]=0;
				
				if (read_subslot!=3)
					rct->read_subslot++;
				else {
					rct->read_subslot=0;
					rct->read_slot=(rct->read_slot+1)%BUF_NUM;
				}
			}

		}
	}
}
/* --------------------------------------------------------------------- */
// Move all real transfers into a thread. 300MHz get too sluggish
// when doing everything in interrupt

int reel_transfer_thread(void* data)
{
	struct reelfpga *rfpga=(struct reelfpga*)data;

	lock_kernel ();
	daemonize ("kreel-transfer");
	current->rt_priority=0;
	current->static_prio=MAX_RT_PRIO+10;
	allow_signal(SIGKILL);
        unlock_kernel ();

	do {
		wait_event_interruptible(rfpga->thread_queue, rfpga->dying || rfpga->data_ready);
		rfpga->data_ready=0; // FIXME
		if (!rfpga->dying)
			reel_transfer_data(rfpga);
	} while(!signal_pending(current) && !rfpga->dying);

	complete_and_exit(&thread_exited,0);
}

static int last=0;
/* --------------------------------------------------------------------- */
// Find out which slots are ready and wake up transfer thread

int reel_irq_handler1(int irq, void *dev_id,
		 struct pt_regs *regs_are_unused)
{
	struct reelfpga *s=(struct reelfpga*)dev_id;
	struct reel_unit *ru;
	struct reel_channel *rct;
	struct reel_dma *rec;
	int n,m,channel,slot,subslot,next_slot;
	int irq_status;
	int ret=IRQ_NONE;
	int needs_handling=0;
	int total_len;
	int diff, rate;

	irq_status=readl(s->regbase+REEL_DMA_IRQSTATUS);
	irq_status&=0xffff;
	if (irq_status==0)
		return ret;

	if (s->dying) {
		writel(0xffff, s->regbase+REEL_DMA_IRQSTATUS); // we don't buy anything anymore
		return IRQ_HANDLED;
	}

	//printk("IRQ %x (j %i, %i) %x\n",irq_status,jiffies,jiffies-last,readl(s->regbase+REEL_DMA_IRQSTATUS+0x30));
	last=jiffies;
	writel(irq_status,s->regbase+REEL_DMA_IRQSTATUS); // Clr IRQs
	for(channel=0;channel<4;channel++) { 	// Check all channels

		ru=s->reels[channel];

		if (ru && ru->feeding && ru->rct ) {
			rct=ru->rct;
			subslot=rct->current_subslot; // next subslot to finish

			for(m=0;m<4;m++) { // max. 4 subslots to check for each channel

				n=channel*4+subslot;
//				printk("%i %i %i %i\n",channel,subslot,m,n);
				if (irq_status&(1<<n)) {

					needs_handling=1;

					slot=rct->current_slot;
					rec=rct->rec[slot];

					rec->finished[subslot]=1;
#if 0
					// Check transfer rate
					diff=(1000*(jiffies-ru->last_transfer_jiffies))/HZ;
					rate=0;
					total_len=rec->len[m];

					if (diff)
						rate=total_len/diff;
					
					printk("DMA%i.%i: %ikB/s\n",channel,n,rate);
					if (rate > 150) {// 150KB/s
						if (ru->latency_fix) {
//							printk("Disable latency fix\n");
							ru->latency_fix=0;
						}
					}
					else if (rate<100) {
						if (!ru->latency_fix) {
							printk("Enable latency fix\n");
						}
						if (rate<50)
							ru->latency_fix=2;
						else
							ru->latency_fix=1;


					}
					printk("DMA%i.%i: %ikB/s %i\n",channel,n,rate,ru->latency_fix );
					ru->rate=rate;
#else
					ru->latency_fix=0;
#endif

#if 0
					printk("INT-Fin(%i) %i.%i.%i\n", n,ru->num,
					       rct->current_slot,subslot);
#endif
					if (rct->flags&REEL_DMA_CONT) { // Restart
						next_slot=(slot+1)%BUF_NUM;
						// Rev 2 loads all subslots on subslot 0 start
						// Rev 3 loads each subslot individually
						if (s->fpga_revision>2)
							start_subslot(rct, next_slot, subslot, ru->latency_fix);
						else
							start_dma(rct,next_slot,ru->latency_fix);

					}

					if (subslot==3) 
						rct->current_slot=(slot+1)%BUF_NUM;

					subslot=(subslot+1)&3;


				} // if irq_status

			} // for m
			rct->current_subslot=subslot;
			ru->last_transfer_jiffies=jiffies;
		} // if ru	       

	} // for channel

	if (needs_handling) {
		s->data_ready=1;
		wake_up(&s->thread_queue);
	}
	ret=IRQ_HANDLED;
	return ret;
}
/* --------------------------------------------------------------------- */
int reel_unit_register(struct reel_unit *ru)
{
        struct dvb_demux *dvbdemux=&ru->demux;
        int ret;

	memset(dvbdemux,0, sizeof(struct dvb_demux));

	dvbdemux->priv = (void *) ru;
	
	if (ru->num<2)
		dvbdemux->filternum = REEL_PIDF01_MAX_PIDS;
	else
		dvbdemux->filternum = REEL_PIDF23_MAX_PIDS;

        dvbdemux->feednum = 256;
        dvbdemux->start_feed = reel_start_feed;
        dvbdemux->stop_feed = reel_stop_feed;
        dvbdemux->write_to_decoder = NULL;

        dvbdemux->dmx.capabilities = (DMX_TS_FILTERING | DMX_SECTION_FILTERING |
                                      DMX_MEMORY_BASED_FILTERING);

        dvb_dmx_init(&ru->demux);

        ru->dmxdev.filternum = 256;
        ru->dmxdev.demux = &dvbdemux->dmx;
        ru->dmxdev.capabilities = 0;

        dvb_dmxdev_init(&ru->dmxdev, &ru->dvb_adapter);

        ru->hw_frontend.source = DMX_FRONTEND_0;

	ret = dvbdemux->dmx.add_frontend(&dvbdemux->dmx, &ru->hw_frontend);

	if (ret < 0) {
		err("reel_unit_register: add_frontend #1 failed");
		return ret;
	}
        
	ru->mem_frontend.source = DMX_MEMORY_FE;
	ret=dvbdemux->dmx.add_frontend (&dvbdemux->dmx, 
					&ru->mem_frontend);
	if (ret<0) {
		err("reel_unit_register: add_frontend #2 failed");
		return ret;
	}
        
	ret=dvbdemux->dmx.connect_frontend (&dvbdemux->dmx, 
					    &ru->hw_frontend);
	if (ret < 0) {
		err("reel_unit_register: connect_frontend failed");
		return ret;
	}
	ru->dmx_used=1;

	// FIXME
//        dvb_net_init(ru->dvb_adapter, &ru->dvb_net, &dvbdemux->dmx);

	return 0;
}
/* --------------------------------------------------------------------- */
void reel_unit_unregister(struct reel_unit *ru)
{
        struct dvb_demux *dvbdemux=&ru->demux;

//	dvb_net_release(&ru->dvb_net);

	if (ru->dmx_used) {
		dvbdemux->dmx.close(&dvbdemux->dmx);
		dvbdemux->dmx.remove_frontend(&dvbdemux->dmx, &ru->hw_frontend);
		dvbdemux->dmx.remove_frontend(&dvbdemux->dmx, &ru->mem_frontend);

		dvb_dmxdev_release(&ru->dmxdev);
		dvb_dmx_release(&ru->demux);
	}
}

/* --------------------------------------------------------------------- */
struct reel_unit* reel_init_unit(struct reelfpga* rfpga, int n, int hw_unit, int nim_id)
{
	struct reel_unit *ru;
	const char *names[]={"Reel 0","Reel 1","Reel 2", "Reel 3", "Reel 4 (CA)"};
	
	err("=== reel_init_unit %i, hw_unit %i, nim id %x",n, hw_unit, nim_id);
	ru=(struct reel_unit*)kmalloc(sizeof(struct reel_unit),GFP_KERNEL);

	if (!ru)
		goto init_fail;

	memset(ru,0,sizeof(struct reel_unit));
	
	sema_init(&ru->fe_mutex,1);
	ru->num=n;
	ru->hw_unit=hw_unit;
	ru->rfpga=rfpga;
	ru->nim_id=nim_id;

	if (n<4) {
        	ru->rct=allocate_dma_channel(rfpga,n);

        	if (!ru->rct)
			goto init_fail;
	}

	dvb_register_adapter(&ru->dvb_adapter, names[n], THIS_MODULE);
	ru->dvb_adapter.priv=ru;

	if (reel_i2c_register(ru) < 0) {
		dvb_unregister_adapter (&ru->dvb_adapter);
		return NULL;       
	}
	
	if (reel_unit_register(ru)) {
		dvb_unregister_adapter (&ru->dvb_adapter);
		kfree(ru);
		err("reel_init_unit: failed");
		return NULL;
	}
	
	// Find the right frontend drivers

	reel_frontend_init(ru);

	// CAMs are standalone but logically on unit 0
	if (n==0) {
		reel_ci_attach(ru);
	}

	return ru;

init_fail:
	if (ru)
		kfree(ru);
	return NULL;
}
/* --------------------------------------------------------------------- */
int reel_free_unit(struct reel_unit *ru)
{
	printk("reel_free_unit %i\n",ru->num);

	if (ru->ca_used)
		reel_ci_detach(ru);

	// FIXME

	reel_unit_unregister(ru);
	if (ru->i2c_used)
		reel_i2c_unregister(ru);

	dvb_unregister_adapter (&ru->dvb_adapter);

	if (ru->rct)
		free_dma_channel(ru->rct);

	kfree(ru);
	return 0;
}
/* --------------------------------------------------------------------- */
void reel_init_tuner_slots(struct reelfpga *rfpga)
{
	int id[2];
	int num_tuners[2];
	int n,unit=0;
	int ts_mode=0;

	// Get an overview of the connected tuners
	reel_gpio_idreg(rfpga, 0, 0); // Powerup both tuner slots
	reel_gpio_idreg(rfpga, 1, 0);
	msleep(1);

	for(n=0;n<2;n++) {
		id[n]=REEL_TUNER_ID(readl(rfpga->regbase+REEL_GPIO_IDS),n);
		num_tuners[n]=REEL_NIM_TUNERS(id[n]);
	}

	for(n=0;n<2;n++) {	
		switch(num_tuners[n]) {
		case 1:
			rfpga->reels[unit]=reel_init_unit(rfpga, unit, n, id[n]);
			set_ts_matrix(rfpga, REEL_SRC_TS(n), REEL_DST_PIDF(unit));
			set_ts_matrix(rfpga, REEL_SRC_PIDF(unit), REEL_DST_DMA(unit));
//			set_ts_matrix(rfpga, REEL_SRC_TS(n), REEL_DST_DMA(unit));
			unit++;
			break;
		case 2:
			rfpga->reels[unit]=reel_init_unit(rfpga, unit, n, id[n]); // HW-Unit 0/1
#if 1
			set_ts_matrix(rfpga, REEL_SRC_TS(n), REEL_DST_PIDF(unit));
			set_ts_matrix(rfpga, REEL_SRC_PIDF(unit), REEL_DST_DMA(unit));
#else
			set_ts_matrix(rfpga, REEL_SRC_TS(n), REEL_DST_CI(0));
			set_ts_matrix(rfpga, REEL_SRC_CI(0), REEL_DST_CI(2));
			set_ts_matrix(rfpga, REEL_SRC_CI(2), REEL_DST_PIDF(unit));
			set_ts_matrix(rfpga, REEL_SRC_PIDF(unit), REEL_DST_DMA(unit));
#endif
			unit++;
			rfpga->reels[unit]=reel_init_unit(rfpga, unit, n+2, id[n]); // HW-Unit 2/3
			set_ts_matrix(rfpga, REEL_SRC_TS(n+2), REEL_DST_PIDF(unit));
			set_ts_matrix(rfpga, REEL_SRC_PIDF(unit), REEL_DST_DMA(unit));
			unit++;
			ts_mode|=(1<<n); // Set latch mode
			break;
		// maybe we have a "case 3:" in the future ;-)
		default:
			break;
		}
	}

	writel(ts_mode, rfpga->regbase+REEL_TS_MODE);

#if 0
	writel(7, rfpga->regbase+REEL_CI_ENABLE);
	reel_ci_switch(rfpga, 2, 3);
	reel_ci_switch(rfpga, 0, 3);
#endif
	rfpga->connected_units=unit;

}
/* --------------------------------------------------------------------- */
static struct reelfpga *rf=NULL;

static int __devinit 
reelfpga_pci_probe( struct pci_dev *pci_dev, const struct pci_device_id *pci_id )
{
	long regstart;
	int irq=REEL_IRQ;
	struct reelfpga *rfpga=NULL;
	int ret,rev;

	pci_read_config_dword(pci_dev, 0x08, &rev);

	if ( (rev&255) < 2) {
		err("reelfpga_pci_probe: Wrong FPGA revision (%02x). Reload with correct bitstream.",rev&255);
			goto probe_fail;
	}

	rfpga=(struct reelfpga *)kmalloc(sizeof(struct reelfpga),GFP_KERNEL);

	if (!rfpga) {
		err("reelfpga_pci_probe: Out of memory");
		goto probe_fail;
	}

	memset(rfpga,0,sizeof(struct reelfpga));

	rfpga->fpga_revision=rev&255;

	if ( pci_enable_device( pci_dev ) ) {
		err("reelfpga_pci_probe: Can't enable PCI device");
		goto probe_fail;
	}

	// Work around cache in pci_resource_start() causing problems with hotplug
	pci_read_config_dword(pci_dev, 0x10, &regstart);
	regstart=regstart&0xffffff00;
	
	printk("FPGA at %x\n",REEL_MEM_START);

	if (!request_mem_region (regstart, 16384, "reelfpga")) {
		err("reelfpga_pci_probe: MMIO resource %lx unavailable",regstart);
		goto probe_fail;
	}
	rfpga->pci_address=regstart;
	rfpga->regbase=ioremap(regstart, 16384);

	if (!rfpga->regbase) {
		err("reelfpga_pci_probe: MMIO mapping at %x failed", regstart);
		release_mem_region(regstart, 16384);
		goto probe_fail;
	}

	init_reel_hw(rfpga);

	sema_init(&rfpga->ci_mutex,1);

	spin_lock_init(&rfpga->lock);
	spin_lock_init(&rfpga->i2c_lock);
	spin_lock_init(&rfpga->hw_lock);
	init_waitqueue_head(&rfpga->thread_queue);
	rfpga->thread_pid = 0;
	rfpga->dying=0;
	rfpga->data_ready=0;
	rfpga->pci_dev=pci_dev;

	if (request_irq(irq, reel_irq_handler1, SA_SHIRQ,
			"reeldvb", rfpga)) {
		err("reelfpga_pci_probe: Can't get IRQ %i",pci_dev->irq);
		iounmap(rfpga->regbase);
		release_mem_region(regstart, 16384);
		goto probe_fail;
	}
	pci_dev->irq=irq;

	reel_ci_init(rfpga);

	reel_init_tuner_slots(rfpga);
	

	// Keep for debugging and other bad times...
	rf=rfpga;

	ret = kernel_thread (reel_transfer_thread, rfpga, 0);
	if (ret<0) {
		printk("reelfpga_pci_probe: failed to start reel transfer thread (%d)\n",ret);
		// FIXME
	}
	printk("Started transfer thread %i\n",ret);
	rfpga->thread_pid=ret;

	pci_set_drvdata( pci_dev, rfpga );

	return 0;

probe_fail:
	if (rfpga) {
		
		kfree(rfpga);
	}
	return -ENODEV;
}

/* --------------------------------------------------------------------- */
static void __devexit reelfpga_pci_remove( struct pci_dev *pci_dev )
{
	struct reelfpga *rfpga = (struct reelfpga*)pci_get_drvdata( pci_dev );
	int n;

	rfpga->dying=1;

	// Kill thread

	if (rfpga->thread_pid) {	       
                if (kill_proc(rfpga->thread_pid, SIGKILL, 1) == -ESRCH) {
                        printk("reelfpga_pci_remove: Transfer thread already dead?!?\n");
                } else 
			wait_for_completion(&thread_exited);
        }
	
	for(n=0;n<4;n++) {
		if (rfpga->reels[n])
			reel_free_unit(rfpga->reels[n]);
	}

	iounmap(rfpga->regbase);
	release_mem_region(rfpga->pci_address, 16384);
	free_irq(pci_dev->irq, rfpga);
        pci_set_drvdata( pci_dev, NULL );
	kfree(rfpga);
}
/* --------------------------------------------------------------------- */

static struct pci_device_id reelfpga_pci_id[] = {
        { REEL_PCI_VID, REEL_PCI_DID, PCI_ANY_ID, PCI_ANY_ID, },
        {0, }
};   

static struct pci_driver reelfpga_driver =
{
        .name           = "reelfpga",
        .id_table       = reelfpga_pci_id,
        .probe          = reelfpga_pci_probe,
        .remove         = __devexit_p(reelfpga_pci_remove),
};
/* --------------------------------------------------------------------- */
static int reelfpga_ioctl (struct inode *inode, struct file *file, unsigned int cmd, unsigned long arg)
{
	int ret = 0;
	unsigned char buf[2048];
	struct reelfpga *rfpga=(struct reelfpga*)file->private_data;
	int fpga_used=0; // FIXME, reconfiguration during DMA transfer might kill the box
	int a;
	
	// ioctls in dumb (non-FPGA) mode
	switch (cmd) {
	case IOCTL_REEL_FPGA_CLEAR:
		if (!fpga_used) {
			reel_fpga_clear_config();
		}
		else
			ret=-EBUSY;
		break;
	case IOCTL_REEL_FPGA_WRITE:
		if (copy_from_user (buf, (void *) arg, 512)) {
                        ret = -EFAULT;
			break;
		}
		if (!fpga_used)
			reel_fpga_write_config(buf,512);
		else
			ret=-EBUSY;
		break;
	case IOCTL_REEL_FPGA_HOTPLUG:
		if (!fpga_used)
			reel_fpga_hotplug();
		else
			ret=-EBUSY;
		break;
	case IOCTL_REEL_SET_DEMUX:
		set_isa_demux(arg);
		break;
	default:
		ret=-ENOIOCTLCMD;
	}

	if (rfpga && ret==-ENOIOCTLCMD) {		
		switch (cmd) {	
		case IOCTL_REEL_CI_SWITCH:
			reel_ci_switch(rfpga,arg&3,(arg>>8)&3); // slot, state
			ret=0;
			break;

		case IOCTL_REEL_SET_MATRIX:
			set_ts_matrix(rfpga, (arg>>8)&0xff,arg&0xff);  // source, dest	 	
			ret=0;
			break;

		case IOCTL_REEL_TUNER_ID:
			a=readl(rfpga->regbase+REEL_GPIO_IDS)&65535;
			if (copy_to_user((void*)arg,&a,sizeof(int)))
				ret = -EFAULT;
                        break;		

		case IOCTL_REEL_CI_ASSOC:			
			ret=reel_ci_change_assoc(rfpga,arg&0xff,(arg>>16)&0xff); // slot, tuner
			break;

		case IOCTL_REEL_CI_GET_STATE:
			ret=reel_ci_get_state(rfpga, arg);
			break;

		case IOCTL_REEL_PUSH_TS:
			if (copy_from_user (buf, (void *) arg, sizeof(int)))
				ret = -EFAULT;
			else {
				a=*(int*)buf;
				if (a<=0 || a>8)
					return -EINVAL;
				if (copy_from_user (buf, (void *) arg, sizeof(int)+188*a))
					ret = -EFAULT;
				else
					ret=reel_push_ts(rfpga,a,buf+4);
			}
			break;
		default:
			ret=-ENOIOCTLCMD;
		}
	}

	return ret;
}
/* --------------------------------------------------------------------- */
int reelfpga_open (struct inode *inode, struct file *file)
{
	int devnum = MINOR (inode->i_rdev);
	struct reelfpga* s=rf;
// FIXME sema	
	if (devnum <0 || devnum>3)
		return -EIO;

//	down (&s->mutex);
//	atomic_inc(&s->opened);
	file->private_data=rf;
//	up (&s->mutex);	
	return 0;
}
/* --------------------------------------------------------------------- */
int reelfpga_release (struct inode *inode, struct file *file)
{
//	struct reelfpga* s=(struct reelfpga*)file->private_data;
//	lock_kernel();	
//	atomic_dec(&s->opened);
//	unlock_kernel();

	return 0;
}
/* --------------------------------------------------------------------- */
static struct file_operations reelfpga_fops =
{
	owner:		THIS_MODULE,
	ioctl:		reelfpga_ioctl,
	open:		reelfpga_open,
//	read:           reelfpga_read,
	release:	reelfpga_release,
};
/* --------------------------------------------------------------------- */
void  reelfpga_init_xtra(void)
{
	int retval,n;

	dbg("Init /dev/reelfpga");
	reel_init_gpios();

	retval= register_chrdev(REELFPGA_MAJOR, "reelfpga", &reelfpga_fops);

	devfs_mk_cdev(MKDEV(REELFPGA_MAJOR, n),
		      S_IFCHR | S_IRUSR | S_IWUSR,
		      "reelfpga%d", 0);
}
/* --------------------------------------------------------------------- */
void  reelfpga_remove_xtra(void)
{
	unregister_chrdev(REELFPGA_MAJOR, "reelfpga");
}

/* --------------------------------------------------------------------- */
void reel_reload(void)
{
	pci_unregister_driver(&reelfpga_driver);
	msleep(100);
	pci_module_init(&reelfpga_driver);
}
/* --------------------------------------------------------------------- */
static int __init reel_init(void) 
{
	reelfpga_init_xtra();
	return pci_module_init(&reelfpga_driver);
}

/* --------------------------------------------------------------------- */

static void __exit reel_exit(void)
{
	reelfpga_remove_xtra();
        return pci_unregister_driver(&reelfpga_driver);	
}

module_init(reel_init);
module_exit(reel_exit);
