
/****************************************************************************
 *
 *  include/asm-arm/errata/arm-errata.S
 *
 *  Copyright (C) 2007 Conexant Systems Inc, USA.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License Version 2 as published by
 *  the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., Franklin St, Fifth Floor, Boston, MA  02111-1307  USA
 *
 ****************************************************************************/
/* $Id$
 ****************************************************************************/

/* 
 * This file was added to correct a bugs in r0p4 version of arm 1176 processors.
 * For more information on the bug, refer to arm 1176 errata, document version 13.0, 
 * date of issue 06-Feb-2007 .
 */

#ifndef __ARM_ERRATA__
#define __ARM_ERRATA__

	.macro invalidate_entire_icache, Rtmp1, Rtmp2		/* Rtmp2 is ignored in case of non-ARM 1176 */
#ifdef CONFIG_ARM1176_ERRATA_FIX

	mov 	\Rtmp1, #0
	mrs 	\Rtmp2, cpsr
	cpsid 	ifa				@ disable interrupts 
	mcr	p15, 0, \Rtmp1, c7, c5, 0 	@ Invalidate entire Instruction Cache
	mcr	p15, 0, \Rtmp1, c7, c5, 0 	@ Invalidate entire Instruction Cache
	mcr	p15, 0, \Rtmp1, c7, c5, 0 	@ Invalidate entire Instruction Cache 
	mcr	p15, 0, \Rtmp1, c7, c5, 0 	@ Invalidate entire Instruction Cache
	msr 	cpsr_cx, \Rtmp2			@ reenable interrupts 
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop 
#else
	mcr	p15, 0, \Rtmp1, c7, c5, 0	
#endif
	.endm

#endif

/****************************************************************************
 * Modifications:
 * $Log$
 *
 ****************************************************************************/ 
	
