/*
 *  linux/include/linux/mtd/cnxtnfc.h
 *
 *  Copyright (C) 2005-2006 Samsung Electronics
 *  Kyungmin Park <kyungmin.park@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __LINUX_MTD_CNXTNFC_H
#define __LINUX_MTD_CNXTNFC_H

#include <linux/spinlock.h>
#include <linux/completion.h>
#include <linux/mtd/bbm.h>

// RK: No BufferRAM. Thats a FIFO on NFC
//#define MAX_BUFFERRAM		2

#define NFC_SPARE_COLUMN_MASK 0x800

/* Scan and identify a CnxtNFC device */
extern int nfc_scan(struct mtd_info *mtd, int max_chips);
/* Free resources held by the CnxtNFC device */
extern void nfc_release(struct mtd_info *mtd);


/**
 * struct nfc_bufferram - CnxtNFC BufferRAM Data
 * @block:		block address in BufferRAM
 * @page:		page address in BufferRAM
 * @valid:		valid flag
 */
// RK: No buffer ram on NFC.
/*struct nfc_bufferram {
	int block;
	int page;
	int valid;
};*/

/**
 * struct nfc_dev - Conexant NFC Private Configuration data
 * @rwbase:		[BOARDSPECIFIC] Base Address of the FIFO to read and write 
 *                    pages to and from NAND Flash.
 * @flashsize:		[INTERN] the size of NAND flash connected to NFC
	@mfr_id:		[INTERN] Manufacturer ID
 * @device_id:		[INTERN] device ID
 * @iodev_config:		[BOARDSPECIFIC] IO Sequencer Device Configuration
 * @chip_select: [INTERN] which nand device to select in a multi chip arrangement
 * @erase_shift:	[INTERN] number of address bits in a block
 * @page_shift:		[INTERN] number of address bits in a page
 * @ppb_shift:		[INTERN] number of address bits in a pages per block
 * @page_mask:		[INTERN] a page per block mask
 
 * @command:		[REPLACEABLE] hardware specific function for writing
 *			commands to the chip
 * @wait:		[REPLACEABLE] hardware specific function for wait on ready
 * @read_page:	[REPLACEABLE] hardware specific function for reading a page from NAND Flash
 * @write_page:	[REPLACEABLE] hardware specific function for writing a page to NAND Flash

 * @mmcontrol:		sync burst read function
 * @block_markbad:	function to mark a block as bad
 * @scan_bbt:		[REPLACEALBE] hardware specific function for scanning
 *			Bad Block Table
 * @chip_lock:		[INTERN] spinlock used to protect access to this
 *			structure and the chip
 * @wq:			[INTERN] wait queue to sleep on if a NFC
 *			operation is in progress
 * @state:		[INTERN] the current state of NFC
 * @page_buf:		data buffer
 * @subpagesize:	[INTERN] holds the subpagesize
 * @ecclayout:		[REPLACEABLE] the default ecc placement scheme
 * @bbm:		[REPLACEABLE] pointer to Bad Block Management
 * @priv:		[OPTIONAL] pointer to private chip data
 */

typedef enum {
	FL_READY,
	FL_READING,
	FL_WRITING,
	FL_ERASING,
	FL_SYNCING,
	FL_LOCKING,
	FL_RESETING,
	FL_OTPING,
	FL_PM_SUSPENDED,
} nand_state_t;

struct nfc_dev {
	void __iomem		*rwbase;
	unsigned int		flashsize;
	unsigned int		mfr_id;
	unsigned int		device_id;
	unsigned int		iodev_config;
   unsigned int      chipselect;
	unsigned int		erase_shift;
	unsigned int		page_shift;
	unsigned int		ppb_shift;	/* Pages per block shift */
	unsigned int		page_mask;
	unsigned int		options;
//	int (*command)(struct mtd_info *mtd, int cmd, loff_t address, size_t len);
	int (*wait)(struct nfc_dev *nfc, int state);
	int (*read_page)(struct nfc_dev *nfc, unsigned char *buffer, int offset);
	int (*write_page)(struct nfc_dev *nfc, const unsigned char *buffer, int offset);
   int (*read_spare)(struct nfc_dev *nfc, unsigned char *buffer, int offset);
	int (*write_spare)(struct nfc_dev *nfc, const unsigned char *buffer, int offset);
   int (*erase_block)(struct nfc_dev *nfc, int offset);
	void (*mmcontrol)(struct nfc_dev *nfc, int sync_read);
	int (*block_markbad)(struct mtd_info *mtd, loff_t ofs);
	int (*check_isbad)(struct nfc_dev *nfc, unsigned int offset);
	int (*scan_bbt)(struct mtd_info *mtd);

	struct completion	complete;
	int			irq;

	spinlock_t		chip_lock;
	wait_queue_head_t	wq;
	nand_state_t		state;
	unsigned char		*page_buf;

	int			subpagesize;
	struct nand_ecclayout	*ecclayout;

	void			*bbm;

	void			*priv;
};

/*
 * Helper macros
 */

/* Check byte access in CnxtNFC */
#define CNXTNFC_CHECK_BYTE_ACCESS(addr)		(addr & 0x1)

/*
 * Options bits
 */
//#define CNXTNFC_HAS_CONT_LOCK		(0x0001)
//#define CNXTNFC_HAS_UNLOCK_ALL		(0x0002)
//#define CNXTNFC_PAGEBUF_ALLOC		(0x1000)

#define CNXTNFC_BADBLOCK_POS       0

/*
 * NAND Flash Manufacturer ID Codes
 */
#define NAND_MFR_TOSHIBA	0x98
#define NAND_MFR_SAMSUNG	0xec
#define NAND_MFR_FUJITSU	0x04
#define NAND_MFR_NATIONAL	0x8f
#define NAND_MFR_RENESAS	0x07
#define NAND_MFR_STMICRO	0x20
#define NAND_MFR_HYNIX		0xad
#define NAND_MFR_MICRON    0x2c

/**
 * struct nand_flash_dev - NAND Flash Device ID Structure
 * @name:	Identify the device type
 * @id:		device ID code
 * @pagesize:	Pagesize in bytes. Either 256 or 512 or 0
 *		If the pagesize is 0, then the real pagesize
 *		and the eraseize are determined from the
 *		extended id bytes in the chip
 * @erasesize:	Size of an erase block in the flash device.
 * @chipsize:	Total chipsize in Mega Bytes
 * @options:	Bitfield to store chip relevant options
 */
struct nand_flash_dev {
	char *name;
	int mfr_id;
	int dev_id;
	unsigned long pagesize;
	unsigned long sparesize;
	unsigned long chipsize;
	unsigned long erasesize;
	unsigned long options;
};

/**
 * struct nand_manufacturers - NAND Flash Manufacturer ID Structure
 * @name:	Manufacturer name
 * @id:		manufacturer ID code of device.
*/
struct nand_manufacturers {
	int id;
	char * name;
};

extern struct nand_flash_dev nand_flash_ids[];
extern struct nand_manufacturers nand_manuf_ids[];

/*****************************************************/
/* CONEXANT NAND FLASH CONTROLLER DEFINITIONS */
/*****************************************************/
#include "nfc_pecos.h"


/* Default Device configuration values */
#define  CNXT_NFC_DEFAULT_CHIPSELECT              0xFE
#define  CNXT_NFC_DEFAULT_RDYBUSYTIMEOUT          0xFFFFFFFF
#define  CNXT_NFC_DEFAULT_FLASHSIZE               0x08 /* NFC_FLASH_SIZE_128MB */
#define  CNXT_NFC_DEFAULT_PAGESIZE                0x00 /* NFC_PAGE_SIZE_512B */
#define  CNXT_NFC_DEFAULT_BUSWIDTH                0x01 /* NFC_BUS_WIDTH_8BIT */
#define  CNXT_NFC_DEFAULT_DEVICETYPE              0x00 /* NFC_DEV_TYPE_8BIT */
#define  CNXT_NFC_DEFAULT_ADDRCYCLES              0x03 /* NFC_ADDR_CYCLES_3CYCLES */
#define  CNXT_NFC_DEFAULT_SPAREAREASIZE           0x00 /* NFC_SPARE_AREA_SIZE_16B */
#define  CNXT_NFC_DEFAULT_ECCON                   0x00 /* NFC_ECC_INIT_OFF */

#define  CNXT_NFC_DEFAULT_ECCTYPE                 0x00 /* NFC_ECC_TYPE_HAMM */
#define  CNXT_NFC_DEFAULT_ECCINCLUDESPARE         0x00 /* NFC_ECC_524_OFF */
#define  CNXT_NFC_DEFAULT_ECCCORRECTENABLE        0x00 /* NFC_ECC_CORRECT_OFF */
#define  CNXT_NFC_DEFAULT_RDYBUSYTIMEOUTENABLE    0x01 /* NFC_RDY_BUSY_TIMEOUT_ENABLE */

/* Default timing parameters */
#define  CNXT_NFC_DEFAULT_WREnablePulseWidth      0x7
#define  CNXT_NFC_DEFAULT_CLEHoldTime             0x7
#define  CNXT_NFC_DEFAULT_WREnablePulseWidthHIGH  0x7
#define  CNXT_NFC_DEFAULT_ALEHoldTime             0x7
#define  CNXT_NFC_DEFAULT_WREnableHIGH2Busy       0x7
#define  CNXT_NFC_DEFAULT_Rdy2RDEnableLOW         0x7
#define  CNXT_NFC_DEFAULT_RDEnablePulseWidth      0x7
#define  CNXT_NFC_DEFAULT_RDEnableHoldTimeHIGH    0x7
#define  CNXT_NFC_DEFAULT_CLE2RDEnable            0x7
#define  CNXT_NFC_DEFAULT_Addr2DataLatch          0x7
#define  CNXT_NFC_DEFAULT_ALE2RDEnable            0x3

/* Status Register bit definitions for Samsung Flash */
#define  FLASH_STATUS_PROGRAM_ERASE_MASK          0x1
#define  FLASH_STATUS_DEV_OPERATION_MASK          0x40     /* 0: Busy   1: Ready*/
#define  FLASH_STATUS_WRITE_PROTECT_MASK          0x80


/*********************************/
/*    Micro Code Declarations    */
/*********************************/
extern unsigned int NFCOpnReadId[];
extern unsigned int NFCOpnReadIdSize;
extern unsigned int NFCOpnReadStatus[];
extern unsigned int NFCOpnReadStatusSize;
extern unsigned int NFCOpnWaitForDevRdy[];
extern unsigned int NFCOpnWaitForDevRdySize;
extern unsigned int NFCOpnReadPage[];
extern unsigned int NFCOpnReadPageSize;
extern unsigned int NFCOpnReadSpare[];
extern unsigned int NFCOpnReadSpareSize;
extern unsigned int NFCOpnReadBadBlkMarker[];
extern unsigned int NFCOpnReadBadBlkMarkerSize;
extern unsigned int NFCOpnReadPageEccON[];
extern unsigned int NFCOpnReadPageEccONSize;
extern unsigned int NFCOpnBlockErase[];
extern unsigned int NFCOpnBlockEraseSize;
extern unsigned int NFCOpnPageWriteEccON[];
extern unsigned int NFCOpnPageWriteEccONSize;
extern unsigned int NFCOpnPageWrite[];
extern unsigned int NFCOpnPageWriteSize;
extern unsigned int NFCOpnSpareWrite[];
extern unsigned int NFCOpnSpareWriteSize;
extern unsigned int NFCOpnReset[];
extern unsigned int NFCOpnResetSize;

extern unsigned int NFCOpnTestUCode[];
extern unsigned int NFCOpnTestUCodeSize;

#endif	/* __LINUX_MTD_CNXTNFC_H */
