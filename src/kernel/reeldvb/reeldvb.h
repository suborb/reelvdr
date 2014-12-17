/*****************************************************************************/
/*
 * DVB-API driver for Reelbox FPGA (4 tuner version)
 *
 * Copyright (C) 2004-2005  Georg Acher, BayCom GmbH (acher at baycom dot de)
 *
 *
 * #include <gpl-header>
 *
 *  $Id: reeldvb.h,v 1.3 2005/09/24 18:41:17 acher Exp $
 *
 */       
/*****************************************************************************/

#ifndef _REELFPGA_H
#define _REELFPGA_H

//! Major/Minor ID for /dev/reelfpga

#define REELFPGA_MAJOR 244

#define REELFPGA_VERSION 0x2000

typedef struct
{
	int num_packets;
	unsigned char buf[8*188];
} reel_ts_packet;

// defines for available ioctl()-values 

// FPGA-download
#define IOCTL_REEL_FPGA_CLEAR  _IO('d', 0x1 ) 
#define IOCTL_REEL_FPGA_WRITE  _IOWR('d', 0x2, char[512] )
#define IOCTL_REEL_FPGA_HOTPLUG _IO('d', 0x3 )
#define IOCTL_REEL_SET_DEMUX _IOWR('d', 0x4, int)  

// circumventing DVB-API for special features
#define IOCTL_REEL_CI_SWITCH _IOW('d', 0x40, int)
#define IOCTL_REEL_SET_MATRIX _IOW('d', 0x41, int)
#define IOCTL_REEL_TUNER_ID _IOR('d', 0x42, int)
#define IOCTL_REEL_LNB_VOLTAGE _IOWR('d', 0x43, int[2])  // Tuner-No, normal(0)/+1V/read(-1)
#define IOCTL_REEL_CI_ASSOC _IOWR('d', 0x44, int)
#define IOCTL_REEL_CI_GET_STATE _IOR('d', 0x45, int)
#define IOCTL_REEL_PUSH_TS _IOR('d', 0x46, reel_ts_packet*)


#define REEL_PCI_VID  0x7ee1   // Reel
#define REEL_PCI_DID  0xc001   // Cool

// NIM types
#define REEL_NIM_SU1278  0      // DVB-S
#define REEL_NIM_CU1216  1      // DVB-C
#define REEL_NIM_TU1216  2      // DVB-T
#define REEL_NIM_DVBS2   3	// DVB-S2
#define REEL_NIM_TWIN_DNBU  8   // 2*DVB-S (STV0299)
#define REEL_NIM_TWIN_DNBUV2  9   // 2*DVB-S (STV0288)

#define REEL_NIM_NONE   31

// Get number of tuners of one board
#define REEL_NIM_TUNERS(x)  (x==REEL_NIM_NONE?0: \
			     (x==REEL_NIM_TWIN_DNBU || x==REEL_NIM_TWIN_DNBUV2?2:1))



#ifdef __KERNEL__

// For PCI-Hotplug

#define REEL_PCI_DEV 0xc
#define REEL_MEM_START 0xfbfe0000
#define REEL_MEM_LEN 0x4000
#define REEL_IRQ 10

// DMA-buffersize

#define BUF_NUM 4
#define BUF_SIZE (4*86*188)  
// BUF_SIZE_M/S must be smaller than BUF_SIZE!
#define BUF_SIZE_M (40*188)  
#define BUF_SIZE_S (10*188)  


// Needed includes for structures

#include <linux/i2c.h>
#include <linux/i2c-algo-bit.h>


#include "dvb_frontend.h"
#include "dvbdev.h"
#include "demux.h"
#include "dvb_demux.h"
#include "dmxdev.h"
#include "dvb_filter.h"
#include "dvb_net.h"
#include "dvb_ca_en50221.h"

#include "reelfpga_regs.h"


struct reel_dma
{
	void* mem[4];
	dma_addr_t dma[4];
	ssize_t len[4];        // length really used for DMA-transfers
	ssize_t len_alloc[4];  // allocated buffer length
	int finished[4];
	
};

struct reel_channel
{
	int channel; // 0...3	
	struct reel_dma *rec[BUF_NUM];
	int current_slot;    // slot waiting to be finished
        int current_subslot;

	int read_slot; // Slot to read data out
	int read_subslot; 
	ssize_t pos;   // read position
	int flags; // sync et al.
	int flags1;
	int overflows;
	int state;
	struct reelfpga *reelfpga;
	wait_queue_head_t wait;
};


struct reel_unit {
        int num;
	int hw_unit;   // 0/2 belong to slot 1, 1/3 to slot 2
	// HW
        struct reel_channel      *rct;
        struct reelfpga          *rfpga;
	struct i2c_adapter       adapter;
	struct i2c_algo_bit_data algo_data;
	int                      i2c_used;

        /* devices */

        struct dvb_device       dvb_dev;
        struct dvb_net          dvb_net;

        struct dmxdev           dmxdev;
        struct dvb_demux        demux;

        struct dmx_frontend     hw_frontend;
        struct dmx_frontend     mem_frontend;
	int                     dmx_used;

        int                     fe_synced; 
        struct semaphore        pid_mutex;

        struct dvb_adapter      dvb_adapter;
	struct dvb_frontend     *dvb_frontend;
        struct semaphore        fe_mutex;
        void                    *priv;
	struct dvb_ca_en50221   ca;
	int                     ca_used;
	
        int feeding;
	int nim_id;
	int lnb_voltage;
	int freq;

	int rate;
	int latency_fix;
	int last_transfer_jiffies;

	int sleeping;
};


struct  reelfpga
{
	struct semaphore mutex;
	atomic_t opened; //!< number of open file descriptors        
	spinlock_t lock;
	struct pci_dev *pci_dev;
	int fpga_revision;
	void *regbase;
	int pci_address; // hotplug
	int ready[2];
	int pos[2];
	int cur_buf;
	int overflow;
	struct reel_unit *reels[5];
	int connected_units;

	// CI Management
	struct semaphore ci_mutex;
	u8 ci_real_powerstate[3];    // Status per slot (0/3/5)
	int ci_powerstate;           // Shadow of TPS2206 register
	int ci_resetstate;           // Status of the three slot reset lines
	int ci_slotstate;            // Detection status for DVB core
	int ci_delay;
	int ci_assoc[6];             // CI Slot association
	int ci_changed;              // Change status for reeldvb

	int ca_old_state[3];         // Change status for DVB CA core per slot
	int ca_slotstate; 
	int ca_slot_enable;          // Enable for DVB CA core


	spinlock_t i2c_lock;
	spinlock_t hw_lock;

	wait_queue_head_t thread_queue;
	pid_t thread_pid;
	int dying;
	int data_ready;
	int nim_count;
	
};

// --------------------------- PROTOTYPES ---------------------------------

// from reel_hw.c
void start_subslot(struct reel_channel *rct, int slot, int subslot, int latency_fix);
void start_dma(struct reel_channel *rct, int slot, int latency_fix);
void stop_dma(struct reel_channel *rct);
struct reel_dma *allocate_dma_descr(struct reelfpga *s, int full_buf_size);
void free_dma_descr(struct reelfpga *s, struct reel_dma *rec);
void init_dma_channel(struct reel_channel *rct);
struct reel_channel *allocate_dma_channel(struct reelfpga *s, int channel);
void free_dma_channel(struct reel_channel *rct);
void init_pidfilter(struct reelfpga* s, int num);
int set_pidfilter(struct reel_unit *ru, int mode, int pid);
void init_reel_hw(struct reelfpga* s);
void set_ts_matrix(struct reelfpga* s, int source, int destination);
int reel_push_ts(struct reelfpga* rfpga, int count, unsigned char *buf);
int reel_gpio_idreg(struct reelfpga* rfpga, int slot, int val);
void set_isa_demux(int value);

// from reel_i2c.c
int reel_i2c_register(struct reel_unit *ru);
void reel_i2c_unregister(struct reel_unit *ru);
int reel_lnbp21_write(struct reel_unit *ru, u8 data);

// from reel_fpga.c
void reel_fpga_write_config(unsigned char* buf, int len);
void reel_init_gpios(void);
void reel_fpga_clear_config(void);
void reel_fpga_hotplug(void);

// From reel_ci.c
void reel_tps2206_send(struct reelfpga *rfpga, int data);
int reel_ci_power(struct reelfpga *rfpga, int slot, int voltage);
void reel_ci_init(struct reelfpga *rfpga);
void reel_ci_attach(struct reel_unit *ru);
void reel_ci_detach(struct reel_unit *ru);
int reel_ci_change_assoc(struct reelfpga *rfpga, int unit, int val);
int reel_ci_switch(struct reelfpga *rfpga, int slot, int state);
int reel_ci_get_state(struct reelfpga *rfpga, int arg);

// From reel_frontend.c
int reel_set_dvbs_voltage(struct dvb_frontend* fe, fe_sec_voltage_t voltage);
void reel_frontend_init(struct reel_unit *ru);
#endif

#endif
