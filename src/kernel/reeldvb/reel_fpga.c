/*****************************************************************************/
/*
 * Firmware download for Reelbox FPGA (4 tuner version)
 *
 * Copyright (C) 2004-2005  Georg Acher, BayCom GmbH (acher at baycom dot de)
 *
 *
 * #include <GPL-header>
 *
 *  $Id: reel_fpga.c,v 1.2 2005/09/10 14:34:16 acher Exp $
 *
 */       
/*****************************************************************************/

#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/timer.h>
#include <linux/module.h>
#include <asm/io.h>
#include <linux/pci.h>
#include <linux/device.h>

#include <linux/delay.h>
#include <linux/scx200_gpio.h>

#include "reeldvb.h"

#define dbg(format, arg...) printk(format"\n", ## arg)
#define err(format, arg...) printk(format"\n", ## arg)

#define FPGA_PROG 19
#define FPGA_CCLK 16
#define FPGA_DIN 18

void reel_reload(void);
/* --------------------------------------------------------------------- */
void reel_init_gpios(void)
{
	int a;
	a=inl(0x9000+0x30); // PMR
        a&=~0x10211; // Enable GPIO19/18/16
        outl(a,0x9030);

	scx200_gpio_set(FPGA_PROG, 1);
	scx200_gpio_configure(FPGA_PROG, ~1, 1);
	scx200_gpio_configure(FPGA_PROG, ~2, 2);

	scx200_gpio_set(FPGA_CCLK, 0);
	scx200_gpio_configure(FPGA_CCLK, ~1, 1);
	scx200_gpio_configure(FPGA_CCLK, ~2, 2);

	scx200_gpio_set(FPGA_DIN, 0);
	scx200_gpio_configure(FPGA_DIN, ~1, 1);
	scx200_gpio_configure(FPGA_DIN, ~2, 2);

	outb(inb(0x4d1)|6,0x4d1); // make IRQ9/10 level sensitive (HACK for buggy BIOS)
}
/* --------------------------------------------------------------------- */
void reel_fpga_clear_config(void)
{
	int a=inl(0x9000+0x30); // PMR
	a&=~0x10211; // Enable GPIO19/18/16
	outl(a,0x9030);
	udelay(10);
	scx200_gpio_set(FPGA_PROG, 1);
	scx200_gpio_configure(FPGA_PROG, ~1, 1);
	scx200_gpio_configure(FPGA_PROG, ~2, 2);
	udelay(10);
	scx200_gpio_set(FPGA_PROG, 0);
	udelay(10);
	scx200_gpio_set(FPGA_PROG, 1);
	scx200_gpio_configure(FPGA_PROG, ~1, 0);
	scx200_gpio_configure(FPGA_PROG, ~4, 4);
	udelay(100);
}
/* --------------------------------------------------------------------- */
void reel_fpga_write_config(unsigned char* buf, int len)
{
	int n;

	for(n=0;n<len;n++) {
		int m;
		unsigned char b;

                b=*buf++;
		for(m=0;m<8;m+=2)
                {
                        if (b&128)
                                scx200_gpio_set(FPGA_DIN, 1);
                        else
                                scx200_gpio_set(FPGA_DIN, 0);
			scx200_gpio_set(FPGA_CCLK, 1);
			scx200_gpio_set(FPGA_CCLK, 0);
                        b=b<<1;
                        if (b&128)
                                scx200_gpio_set(FPGA_DIN, 1);
                        else
                                scx200_gpio_set(FPGA_DIN, 0);
			scx200_gpio_set(FPGA_CCLK, 1);
			scx200_gpio_set(FPGA_CCLK, 0);
                        b=b<<1;
                }		
	}
}
/* --------------------------------------------------------------------- */
// Ugly hack: Allow user-issued delayed FPGA detection and init FPGA-PCI

// From PCI hotplug
extern unsigned int pci_do_scan_bus(struct pci_bus *bus);
extern int  pci_scan_slot(struct pci_bus *bus, int devfn);
extern int pci_proc_attach_device(struct pci_dev *dev);

void reel_fpga_hotplug(void)
{
	struct pci_dev *dev = NULL,*dev1;

	dev=pci_find_device(REEL_PCI_VID,REEL_PCI_DID,NULL);
	if (dev==NULL) { // FPGA not found
		int nr;
		dev1=pci_find_device(0x1078,0x0001, NULL); // Host bridge
		if (dev1) {

			nr=pci_scan_slot(dev1->bus,PCI_DEVFN(REEL_PCI_DEV,0));			

			if (nr) {

				pci_bus_add_devices(dev1->bus);

				dev = pci_find_slot(dev1->bus->number, PCI_DEVFN(REEL_PCI_DEV,0));
				if (dev) {

					dev->resource[0].start=REEL_MEM_START;
					dev->resource[0].end  =REEL_MEM_START+REEL_MEM_LEN;
					dev->resource[0].flags=0;
					
					dev->irq = REEL_IRQ;
					pci_enable_device(dev); 

					pci_proc_attach_device(dev);
				}
				err("REELFPGA detected");
//				reel_reload();
				err("REELFPGA reloaded");
			}
		}
	}
	// Reload FPGA config space
	if (dev) {
		pci_write_config_dword(dev, 0x10, REEL_MEM_START);
		pci_write_config_dword(dev, 0x04, 0x02000006); // Target+Master enable
		pci_write_config_dword(dev, 0x0c, 0x00004000); // Latency
		pci_write_config_dword(dev, 0x3c, 0x10100200|REEL_IRQ);
		reel_reload();
		err("REELFPGA reloaded");
		{
			int a=inl(0x9000+0x30); // PMR
			a|=0x200; // Disable GPIO19
			outl(a,0x9030);
		}
	}
}
/* --------------------------------------------------------------------- */
