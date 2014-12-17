/* dvbloop - A DVB Loopback Device
 * Copyright (C) 2007 Deti Fliegl
 -----------------------------------------
 * File: dvblo_adap_ca.c
 * Desc: Support for virtual DVB adapters
 * Date: October 2007
 * Author: Deti Fliegl <deti@fliegl.de>
 *
 * This file is released under the GPLv2.
 */

/* avoid definition of __module_kernel_version in the resulting object file */
#define __NO_VERSION__

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/spinlock.h>
#include <linux/init.h>
#include <linux/errno.h>
#include <linux/version.h>
#include <linux/vmalloc.h>

#include "dvblo.h"
#include "dvblo_adap.h"
#include "dvblo_adap_ca.h"  

#define DBGLEV_ADAP_CA1		(DBGLEV_1<<DBGLEV_ADAP_CA)
#define DBGLEV_ADAP_CA2		(DBGLEV_2<<DBGLEV_ADAP_CA)
#define DBGLEV_ADAP_CA3		(DBGLEV_3<<DBGLEV_ADAP_CA)

void ci_get_data(struct dvb_ringbuffer *cibuf, u8 *data, int len)
{
	if (dvb_ringbuffer_free(cibuf) < len + 2)
		return;
		
	if(len > CA_TPDU_MAX)
		return;

	dprintk(DBGLEV_ADAP_CA1, "ci_get_data len %d\n",len );
		
	DVB_RINGBUFFER_WRITE_BYTE(cibuf, len >> 8);
	DVB_RINGBUFFER_WRITE_BYTE(cibuf, len & 0xff);
	dvb_ringbuffer_write(cibuf, data, len);
	wake_up_interruptible(&cibuf->queue);
}

/******************************************************************************
 * CI link layer file ops
 ******************************************************************************/

static int ci_ll_init(struct dvb_ringbuffer *cirbuf, struct dvb_ringbuffer *ciwbuf, int size)
{
	struct dvb_ringbuffer *tab[] = { cirbuf, ciwbuf, NULL }, **p;
	void *data;

	for (p = tab; *p; p++) {
		data = vmalloc(size);
		if (!data) {
			while (p-- != tab) {
				vfree(p[0]->data);
				p[0]->data = NULL;
			}
			return -ENOMEM;
		}
		dvb_ringbuffer_init(*p, data, size);
	}
	return 0;
}

static void ci_ll_flush(struct dvb_ringbuffer *cirbuf, struct dvb_ringbuffer *ciwbuf)
{
	dvb_ringbuffer_flush_spinlock_wakeup(cirbuf);
	dvb_ringbuffer_flush_spinlock_wakeup(ciwbuf);
}

static void ci_ll_release(struct dvb_ringbuffer *cirbuf, struct dvb_ringbuffer *ciwbuf)
{
	vfree(cirbuf->data);
	cirbuf->data = NULL;
	vfree(ciwbuf->data);
	ciwbuf->data = NULL;
}

static int ci_ll_reset(struct dvb_ringbuffer *cibuf, struct file *file,
		       int slots, ca_slot_info_t *slot)
{
	int i;
	int len = 0;
	u8 msg[8] = { 0x00, 0x06, 0x00, 0x00, 0xff, 0x02, 0x00, 0x00 };

	for (i = 0; i < CA_MAX_SLOTS; i++) {
		if (slots & (1 << i))
			len += 8;
	}

	if (dvb_ringbuffer_free(cibuf) < len)
		return -EBUSY;

	for (i = 0; i < CA_MAX_SLOTS; i++) {
		if (slots & (1 << i)) {
			msg[2] = i;
			dvb_ringbuffer_write(cibuf, msg, 8);
			slot[i].flags = 0;
		}
	}

	return 0;
}

static ssize_t ci_ll_write(struct dvb_ringbuffer *cibuf, struct file *file,
			   const char __user *buf, size_t count, loff_t *ppos)
{
	int free;
	int non_blocking = file->f_flags & O_NONBLOCK;
	char *page = (char *)__get_free_page(GFP_USER);
	int res;

	if (!page)
		return -ENOMEM;

	res = -EINVAL;
	if (count > CA_TPDU_MAX)
		goto out;

	res = -EFAULT;
	if (copy_from_user(page, buf, count))
		goto out;

	free = dvb_ringbuffer_free(cibuf);
	if (count + 2 > free) {
		res = -EWOULDBLOCK;
		if (non_blocking)
			goto out;
		res = -ERESTARTSYS;
		if (wait_event_interruptible(cibuf->queue,
					     (dvb_ringbuffer_free(cibuf) >= count + 2)))
			goto out;
	}

	DVB_RINGBUFFER_WRITE_BYTE(cibuf, count >> 8);
	DVB_RINGBUFFER_WRITE_BYTE(cibuf, count & 0xff);

	res = dvb_ringbuffer_write(cibuf, page, count);
out:
	free_page((unsigned long)page);
	return res;
}

static ssize_t ci_ll_read(struct dvb_ringbuffer *cibuf, struct file *file,
			  char __user *buf, size_t count, loff_t *ppos)
{
	int avail;
	int non_blocking = file->f_flags & O_NONBLOCK;
	ssize_t len;

	if (!cibuf->data || !count)
		return 0;
	if (non_blocking && (dvb_ringbuffer_empty(cibuf)))
		return -EWOULDBLOCK;
	if (wait_event_interruptible(cibuf->queue,
				     !dvb_ringbuffer_empty(cibuf)))
		return -ERESTARTSYS;
	avail = dvb_ringbuffer_avail(cibuf);
	if (avail < 4)
		return 0;
	len = DVB_RINGBUFFER_PEEK(cibuf, 0) << 8;
	len |= DVB_RINGBUFFER_PEEK(cibuf, 1);
	if (avail < len + 2 || count < len)
		return -EINVAL;
	DVB_RINGBUFFER_SKIP(cibuf, 2);

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,27)
	return dvb_ringbuffer_read_user(cibuf, buf, len);
#else
	return dvb_ringbuffer_read(cibuf, buf, len, 1);
#endif
}

static int dvb_ca_open(struct inode *inode, struct file *file)
{
	struct dvb_device *dvbdev = file->private_data;
	struct dvblo *dvblo = dvbdev->priv;
	int err = dvb_generic_open(inode, file);

	dprintk(DBGLEV_ADAP_CA1, "dvblo:%p %d\n",dvblo, err);

	if (err < 0)
		return err;
	ci_ll_flush(&dvblo->ci_rbuffer, &dvblo->ci_wbuffer);
	return 0;
}

static unsigned int dvb_ca_poll (struct file *file, poll_table *wait)
{
	struct dvb_device *dvbdev = file->private_data;
	struct dvblo *dvblo = dvbdev->priv;
	struct dvb_ringbuffer *rbuf = &dvblo->ci_rbuffer;
	struct dvb_ringbuffer *wbuf = &dvblo->ci_wbuffer;
	unsigned int mask = 0;

	dprintk(DBGLEV_ADAP_CA1, "dvblo:%p\n",dvblo);

	poll_wait(file, &rbuf->queue, wait);
	poll_wait(file, &wbuf->queue, wait);

	if (!dvb_ringbuffer_empty(rbuf))
		mask |= (POLLIN | POLLRDNORM);

	if (dvb_ringbuffer_free(wbuf) > 1024)
		mask |= (POLLOUT | POLLWRNORM);

	return mask;
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,35)
static int dvb_ca_ioctl(struct file *file, unsigned int cmd, void *parg)
#else
static int dvb_ca_ioctl(struct inode *inode, struct file *file,
		 unsigned int cmd, void *parg)
#endif
{
	struct dvb_device *dvbdev = file->private_data;
	struct dvblo *dvblo = dvbdev->priv;
	unsigned long arg = (unsigned long) parg;

	dprintk(DBGLEV_ADAP_CA1, "dvblo:%p\n",dvblo);

	switch (cmd) {
	case CA_RESET:
		dvblo_set_event (dvblo, EV_CA_RESET);
		if(arg) {
			int ret=ci_ll_reset(&dvblo->ci_wbuffer, file, arg, dvblo->ca.info);
			dvblo_set_event (dvblo, EV_CA_WRITE);
			return ret;
		}
		break;
	case CA_GET_CAP:
	{
		memcpy(parg, &dvblo->ca.cap, sizeof(ca_caps_t));
		break;
	}

	case CA_GET_SLOT_INFO:
	{
		ca_slot_info_t *info=(ca_slot_info_t *)parg;

		if (info->num > (CA_MAX_SLOTS-1))
			return -EINVAL;
		memcpy(info, &dvblo->ca.info[info->num], sizeof(ca_slot_info_t));
		break;
	}
#if 0
	case CA_SET_DESCR:
		dvblo_set_event (dvblo, EV_CA_DESCR);
		break;
		
	case CA_SET_PID:
		dvblo_set_event (dvblo, EV_CA_PID);
		break;
#endif
	default:
		return -EINVAL;
	}
	return 0;
}

static ssize_t dvb_ca_write(struct file *file, const char __user *buf,
			    size_t count, loff_t *ppos)
{
	struct dvb_device *dvbdev = file->private_data;
	struct dvblo *dvblo = dvbdev->priv;
	int ret;

	dprintk(DBGLEV_ADAP_CA1, "dvblo:%p write\n",dvblo);
	ret=ci_ll_write(&dvblo->ci_wbuffer, file, buf, count, ppos);
	dvblo_set_event (dvblo, EV_CA_WRITE);
	return ret;
}

static ssize_t dvb_ca_read(struct file *file, char __user *buf,
			   size_t count, loff_t *ppos)
{
	struct dvb_device *dvbdev = file->private_data;
	struct dvblo *dvblo = dvbdev->priv;

	dprintk(DBGLEV_ADAP_CA1, "dvblo:%p read %zd\n",dvblo, count);
	return ci_ll_read(&dvblo->ci_rbuffer, file, buf, count, ppos);
}

static struct file_operations dvb_ca_fops = {
	.owner		= THIS_MODULE,
	.read		= dvb_ca_read,
	.write		= dvb_ca_write,
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,35)
	.unlocked_ioctl = dvb_generic_ioctl,
#else
	.ioctl		= dvb_generic_ioctl,
#endif
	.open		= dvb_ca_open,
	.release	= dvb_generic_release,
	.poll		= dvb_ca_poll,
};

static struct dvb_device dvbdev_ca = {
	.priv		= NULL,
	.users		= 1,
	.writers	= 1,
	.fops		= &dvb_ca_fops,
	.kernel_ioctl	= dvb_ca_ioctl,
};

int dvblo_ca_register(struct dvblo *dvblo)
{
	return dvb_register_device(&dvblo->dvb.adap, &dvblo->dvb.ca_dev,
				   &dvbdev_ca, dvblo, DVB_DEVICE_CA);
}

void dvblo_ca_unregister(struct dvblo *dvblo)
{
	dvb_unregister_device(dvblo->dvb.ca_dev);
}

int dvblo_ca_init(struct dvblo* dvblo)
{
	return ci_ll_init(&dvblo->ci_rbuffer, &dvblo->ci_wbuffer, 8192);
}

void dvblo_ca_exit(struct dvblo* dvblo)
{
	ci_ll_release(&dvblo->ci_rbuffer, &dvblo->ci_wbuffer);
}
