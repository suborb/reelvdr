/*****************************************************************************/
/*
 * Register definitions for Reelbox FPGA (4 tuner version)
 *
 * Copyright (C) 2004-2005  Georg Acher, BayCom GmbH (acher at baycom dot de)
 *
 *
 * #include <GPL-header>
 *
 *  $Id: reelfpga_regs.h,v 1.3 2005/09/24 18:41:17 acher Exp $
 *
 */       
/*****************************************************************************/


#ifndef _INCLUDE_REELFPGA_REGS_H
#define _INCLUDE_REELFPGA_REGS_H

//-------------------------------------------------------------------------
// GPIO
//-------------------------------------------------------------------------

#define REEL_GPIOBASE   0x000
#define REEL_GPIO_IDS   (REEL_GPIOBASE+0x0)       // R
#define REEL_GPIO_CIPWR (REEL_GPIOBASE+0x4)       // RW
#define REEL_GPIO_I2C   (REEL_GPIOBASE+0x8)       // RW
#define REEL_GPIO_DSQ   (REEL_GPIOBASE+0xc)       // R
#define REEL_GPIO_IDOUT (REEL_GPIOBASE+0x10)      // RW

// for REEL_GPIO_IDS

#define REEL_PWR_OVERCURRENT(x) (((x)>>16)&1)

// Others use EEPROMs for identification, but 5 resistors are enough for a start
#define REEL_PWR_TUNER1_ID(x)   (((x))&31)
#define REEL_PWR_TUNER2_ID(x)   (((x)>>8)&31)
#define REEL_TUNER_ID(x,slot)  (((x)>>(8*slot))&31)


// for REEL_GPIO_CIPWR
#define REEL_PWR_DATA  1
#define REEL_PWR_CLK   2
#define REEL_PWR_LATCH 4
#define REEL_PWR_RESET 8

// for REEL_GPIO_I2C

#define REEL_I2C_SCL1 1
#define REEL_I2C_SDA1 2
#define REEL_I2C_SCL2 4
#define REEL_I2C_SDA2 8
#define REEL_I2C_SCL3 0x10
#define REEL_I2C_SDA3 0x20
#define REEL_I2C_SCL4 0x40
#define REEL_I2C_SDA4 0x80
#define REEL_I2C_RSCL(x,bus) (((x)>>(8+2*bus))&1)
#define REEL_I2C_RSDA(x,bus) (((x)>>(9+2*bus))&1)

// for REEL_GPIO_DSQ
#define REEL_I2C_DSQ1(x) ((x)&1)
#define REEL_I2C_DSQ2(x) (((x)>>1)&1)

//-------------------------------------------------------------------------
// CI-Ctrl
//-------------------------------------------------------------------------

#define REEL_CIBASE    0x100
#define REEL_CI_CA    (REEL_CIBASE+0x0)           // RW
#define REEL_CI_DATA  (REEL_CIBASE+0x4)           // R

#define REEL_CI_OE   0x0100
#define REEL_CI_LOW_ADDR      0x0200      
#define REEL_CI_HIGH_ADDR     0x0400
#define REEL_CI_CES  0x0800
#define REEL_CI_CTRL 0x1000
#define REEL_CI_BUF1 0x2000
#define REEL_CI_BUF2 0x4000
#define REEL_CI_BUFD 0x8000

//-------------------------------------------------------------------------
// DMA
//-------------------------------------------------------------------------

#define REEL_DMABASE 0x200
#define REEL_DMA0 (REEL_DMABASE+0x0)
#define REEL_DMA1 (REEL_DMABASE+0x4)
#define REEL_DMA2 (REEL_DMABASE+0x8)
#define REEL_DMA3 (REEL_DMABASE+0xc)

#define REEL_DMA_ENABLED (1)      // 0x001
#define REEL_DMA_CONT    (1<<1)   // 0x002
#define REEL_DMA_IOC     (1<<2)   // 0x004
#define REEL_DMA_SYNC    (1<<3)   // 0x008
#define REEL_DMA_VALID   (1<<4)   // 0x010
#define REEL_DMA_RUNNING (1<<5)   // 0x020
#define REEL_DMA_FINISHED (1<<6)  // 0x040
#define REEL_DMA_SUBSLOT (1<<8)   // 0x100

#define REEL_DMA_IRQSTATUS (REEL_DMABASE+0x10)

// R/W registers
#define REEL_DMA_ADDR00 (REEL_DMABASE+0x80)
#define REEL_DMA_LEN00  (REEL_DMABASE+0x84)
#define REEL_DMA_ADDR01 (REEL_DMABASE+0x88)
#define REEL_DMA_LEN01  (REEL_DMABASE+0x8c)
#define REEL_DMA_ADDR02 (REEL_DMABASE+0x90)
#define REEL_DMA_LEN02  (REEL_DMABASE+0x94)
#define REEL_DMA_ADDR03 (REEL_DMABASE+0x98)
#define REEL_DMA_LEN03  (REEL_DMABASE+0x9c)

// And so on for 3 more channels

//-------------------------------------------------------------------------
// Matrix
//-------------------------------------------------------------------------

#define REEL_MATRIXBASE   0x300

// Destinations

#define REEL_DST_DMA0     0
#define REEL_DST_PIDF0    1
#define REEL_DST_DMA1     2
#define REEL_DST_PIDF1    3
#define REEL_DST_DMA2     4
#define REEL_DST_PIDF2    5
#define REEL_DST_DMA3     6
#define REEL_DST_PIDF3    7
#define REEL_DST_CI0      8
#define REEL_DST_X0       9
#define REEL_DST_CI1      10
#define REEL_DST_X1       11
#define REEL_DST_CI2      12
#define REEL_DST_X2       13

#define REEL_DST_DMA(n)   ((n)*2)
#define REEL_DST_PIDF(n)  (1+(n)*2)
#define REEL_DST_CI(n)    (8+(n)*2)
#define REEL_DST_X(n)     (9+(n)*2)

// Source definitions for Matrix

#define REEL_SRC_TS0   0
#define REEL_SRC_PIDF0 1
#define REEL_SRC_TS1   2
#define REEL_SRC_PIDF1 3
#define REEL_SRC_TS2   4
#define REEL_SRC_PIDF2 5
#define REEL_SRC_TS3   6
#define REEL_SRC_PIDF3 7
#define REEL_SRC_CI0   8
#define REEL_SRC_X0    9
#define REEL_SRC_CI1   10
#define REEL_SRC_X1    11
#define REEL_SRC_CI2   12
#define REEL_SRC_X2    13

#define REEL_SRC_TS(n)   ((n)*2)
#define REEL_SRC_PIDF(n) (1+(n)*2)
#define REEL_SRC_CI(n)   (8+(n)*2)
#define REEL_SRC_X(n)    (9+(n)*2)

// Enable clock/data output for CI slot (only writeable)
#define REEL_CI_ENABLE (REEL_MATRIXBASE+0x40)

// TS latch mode (1 for Twin-Tuner) (only writeable)
#define REEL_TS_MODE (REEL_MATRIXBASE+0x44)

//-------------------------------------------------------------------------
// PID-Filter
//-------------------------------------------------------------------------

#define REEL_PIDF01_MAX_PIDS 8192
#define REEL_PIDF23_MAX_PIDS (12*2)

#define REEL_PIDF0BASE 0x1000
#define REEL_PIDF1BASE 0x1800

#define REEL_PIDF_CTRL 0
#define REEL_PIDF_RAM  0x400

// for REEL_PIDF_CTRL

#define REEL_PIDF_DISABLE 1
#define REEL_PIDF_INVERT  2
#define REEL_PIDF_BYPASS  4

// Simple PIDF

#define REEL_PIDF2BASE 0x400
#define REEL_PIDF3BASE 0x480

#define REEL_PIDF23_RAMA 0x40
#define REEL_PIDF23_RAMB 0x60

//-------------------------------------------------------------------------
// Reserved for eXtension modules
//-------------------------------------------------------------------------
#define REEL_X0_BASE 0x500 
#define REEL_X1_BASE 0x580 
#define REEL_X2_BASE 0x600 

#endif
