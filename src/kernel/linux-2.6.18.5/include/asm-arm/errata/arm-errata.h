
/****************************************************************************
 *
 *  include/asm-arm/errata/arm-errata.h
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

#ifndef __ARM_ERRATA_H__
#define __ARM_ERRATA_H__

/*
        invalidate_entire_icache ,  Noida BSP - 10/04/2007 , Arm-errata
        For more explaination look into include/arm/errata/arm-errata.S
*/

#ifdef CONFIG_ARM1176_ERRATA_FIX 

#define invalidate_entire_icache(Rtmp1, Rtmp2)    \
	asm ( " mov     %0, #0                                                          \n"	\
        "       mrs     %1, cpsr                                                        \n"	\
        "       cpsid   ifa                     @ disable interrupts                    \n"	\
        "       mcr     p15, 0, %0, c7, c5, 0   @ Invalidate entire Instruction Cache   \n"	\
        "       mcr     p15, 0, %0, c7, c5, 0   @ Invalidate entire Instruction Cache   \n"	\
        "       mcr     p15, 0, %0, c7, c5, 0   @ Invalidate entire Instruction Cache   \n"	\
        "       mcr     p15, 0, %0, c7, c5, 0   @ Invalidate entire Instruction Cache   \n"	\
        "       msr     cpsr_cx, %1             @ reenable interrupts                   \n"	\
        "       nop                                                                     \n"	\
        "       nop                                                                     \n"	\
        "       nop                                                                     \n"	\
        "       nop                                                                     \n"	\
        "       nop                                                                     \n"	\
        "       nop                                                                     \n"	\
        "       nop                                                                     \n"	\
        "       nop                                                                     \n"	\
        "       nop                                                                     \n"	\
        "       nop                                                                     \n"	\
        "       nop                                                                     "	\
	:											\
	: "r" (Rtmp1), "r" (Rtmp2)								\
	: "cc" )

#else

#define invalidate_entire_icache(Rtmp1, Rtmp2)     \
	asm ( "  mcr    p15, 0, %0, c7, c5, 0"							\
	:											\
	: "r" (Rtmp1)										\
	: "cc" )

#endif	

#endif

/****************************************************************************
 * Modifications:
 * $Log$
 *
 ****************************************************************************/ 

