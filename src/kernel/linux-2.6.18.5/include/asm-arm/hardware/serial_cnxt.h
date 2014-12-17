/*
 *  linux/include/asm-arm/hardware/serial_rio.h
 *
 *  Internal header file for CNXT  serial ports
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
 */
#ifndef ASM_ARM_HARDWARE_SERIAL_CNXT_H
#define ASM_ARM_HARDWARE_SERIAL_CNXT_H

/*
 *  UART Register Offsets.
 */
#define CNXT_BRDL                       0x0   /* lower byte of baud rate  divisor */
#define CNXT_BRDH                       0x4   /* lower byte of baud rate  divisor */
#define CNXT_FIFO                       0x0
#define CNXT_IRQE                       0x4
#define CNXT_FIFC                       0x8
#define CNXT_FRMC                       0xc
#define CNXT_STAT                       0x14
#define CNXT_IRLVL                      0x18
#define CNXT_TXSTA                      0x28
#define CNXT_RXSTA                      0x2c

#define CNXT_EXP                        0x30  /* Fractional Baud Rate Divisor */
                                              /* 2-bit for regular UART */
                                              /* 6-bit for HS UART */

#define CNXT_EXCTL                      0x3C  /* Expanded Control Register */
                                              /* HS UART */

#define CNXT_IRQE_ERFE   0x80
#define CNXT_IRQE_TIDE   0x40
#define CNXT_IRQE_TSRE   0x20
#define CNXT_IRQE_RBKE   0x10
#define CNXT_IRQE_FREE   0x08
#define CNXT_IRQE_PAEE   0x04
#define CNXT_IRQE_RFOE   0x02
#define CNXT_IRQE_RSRE   0x01

#define CNXT_FIFC_RFT_ONE  0x00
#define CNXT_FIFC_RFT_FOUR 0x40
#define CNXT_FIFC_RFT_EIGHT 0x80
#define CNXT_FIFC_RFT_TWLV  0xc0
#define CNXT_FIFC_TFT_FOUR  0x00
#define CNXT_FIFC_TFT_EIGHT 0x10
#define CNXT_FIFC_TFT_TWLV  0x20
#define CNXT_FIFC_TFT_SXTN  0x30
#define CNXT_FIFC_TFC       0x04
#define CNXT_FIFC_RFC       0x02

#define CNXT_FRMC_BDS    0x80
#define CNXT_FRMC_TBK    0x40
#define CNXT_FRMC_POR    0x20
#define CNXT_FRMC_EOP    0x10
#define CNXT_FRMC_PEN    0x08
#define CNXT_FRMC_SBS    0x04
#define CNXT_FRMC_LBM    0x02
#define CNXT_FRMC_FRS    0x01

#define CNXT_STAT_ERF    0x80
#define CNXT_STAT_TID    0x40
#define CNXT_STAT_TSR    0x20
#define CNXT_STAT_RBK    0x10
#define CNXT_STAT_FRE    0x08
#define CNXT_STAT_PAE    0x04
#define CNXT_STAT_RFO    0x02
#define CNXT_STAT_RSR    0x01

#define CNXT_EXCTL_HSM   0x80
#define CNXT_EXCTL_OSP   0x40
#define CNXT_EXCTL_RTS   0x20
#define CNXT_EXCTL_DMATX 0x10
#define CNXT_EXCTL_DMARX 0x08
#define CNXT_EXCTL_FCEN  0x04
#define CNXT_EXCTL_RXFC_ONE   0x00
#define CNXT_EXCTL_RXFC_FOUR  0x01
#define CNXT_EXCTL_RXFC_EIGHT 0x02
#define CNXT_EXCTL_RXFC_FRTN  0x03

#define CNXT_STAT_ANY    (CNXT_STAT_RBK | CNXT_STAT_FRE | \
                         CNXT_STAT_PAE | CNXT_STAT_RFO)


#endif
