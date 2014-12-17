/*
 * <LIC_AMD_STD>
 * Copyright (c) <years> Advanced Micro Devices, Inc.
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 * 
 * </LIC_AMD_STD>
 * <CTL_AMD_STD>
 * </CTL_AMD_STD>
 * <DOC_AMD_STD>
 *  CS5535 documentation available from AMD.
 * </DOC_AMD_STD>
 */

#ifndef __OS_INC_H__
#define __OS_INC_H__

#include <linux/types.h> 


//#define AUDIO_DEBUG TRUE

//
//      DMA Buffer constants
//
#define DEFAULT_FRAGMENT_SIZE               8192
#define DEFAULT_NUMBER_OF_FRAGMENTS            8
#define DEFAULT_DMA_BUFFER_SIZE           DEFAULT_NUMBER_OF_FRAGMENTS*DEFAULT_FRAGMENT_SIZE
                                                            
#define SIZE_OF_EACH_PRD_BLOCK            64    //  Each PRD entry will point to 32 bytes (8 samples) of the DMA buffer


//
//  Device and Vendor IDs - Needed like this for OSS only
//
#define CYRIX_VENDOR_ID     0x1078   
#define NATIONAL_VENDOR_ID  0x100B   

//
//       Audio  Device IDs
//
#define CX5530_DEV_ID    0x0103
#define SC1200_DEV_ID    0x0503
#define CS5535_DEV_ID    0x002E

#include <linux/version.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/smp_lock.h>
//#include <linux/wrapper.h>
#include <linux/string.h>
#include <linux/ctype.h>
#include <linux/ioport.h>
#include <linux/delay.h>
#include <linux/sound.h>
#include <linux/slab.h>
#include <linux/soundcard.h>
#include <linux/pci.h>
#include <linux/spinlock.h>
#include <asm/io.h>
#include <asm/dma.h>
#include <linux/init.h>
#include <linux/poll.h>
#include <linux/reboot.h>
#include <asm/uaccess.h>
#include <asm/hardirq.h>
#include <linux/bitops.h>

#include <linux/pm.h>

//---------------------------
//        OS-Specific
//---------------------------

/* printing messages */

void OS_DbgMsg( unsigned char *szFormat, ... );

unsigned long   OS_ReadPortULong        (unsigned short Port);
unsigned short  OS_ReadPortUShort       (unsigned short Port);
unsigned char   OS_ReadPortUChar        (unsigned short Port);
void            OS_WritePortULong       (unsigned short Port, unsigned long  Data);
void            OS_WritePortUShort      (unsigned short Port, unsigned short Data);
void            OS_WritePortUChar       (unsigned short Port, unsigned char  Data);

void*           OS_AllocateMemory       (unsigned long Size);
void*           OS_AllocateDMAMemory    (void* pAdapterObject, unsigned long Size, unsigned long *PhysicalAddress);
void            OS_FreeDMAMemory        (void* pAdapterObject, void* VirtualAddress, unsigned long PhysicalAddress, unsigned long Size);

unsigned char   OS_CopyFromUser         (unsigned char *DMABuffer,  unsigned char *UserBuffer, unsigned long Count);
unsigned char   OS_CopyToUser           (unsigned char *UserBuffer, unsigned char *DMABuffer,  unsigned long Count);

unsigned long   OS_VirtualAddress       (unsigned long PhysAddress);
unsigned long   OS_Get_F3BAR0_Virt      (unsigned long BAR_Phys);
void            OS_Free                 (unsigned char *pucBuff);
void            OS_Sleep                (unsigned long Delay);

void            OS_InitSpinLock         (void);
void            OS_SpinLock             (void);
void            OS_SpinUnlock           (void);

#endif  //  __OS_INC_H__
