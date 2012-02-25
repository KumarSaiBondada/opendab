/* -*- linux-c -*- ***********************************************************/
/*
 *      wavefinder.c  --  Psion WaveFinder driver for Linux 2.6 kernel
 *
 *      Copyright (C) 2005-2010 David Crawley <degressive@yahoo.co.uk>
 *
 *      This program is free software; you can redistribute it and/or modify
 *      it under the terms of the GNU General Public License as published by
 *      the Free Software Foundation; either version 3 of the License, or
 *      (at your option) any later version.
 *
 *      This program is distributed in the hope that it will be useful,
 *      but WITHOUT ANY WARRANTY; without even the implied warranty of
 *      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *      GNU General Public License for more details.
 *
 *      You should have received a copy of the GNU General Public License
 *      along with this program; if not, If not, see <http://www.gnu.org/licenses/>
 *      or write to the Free Software Foundation, Inc., 675 Mass Ave, Cambridge,
 *      MA 02139, USA.
 * *
 *      Based partly on dabusb.c by Deti Fliegl and audio.c by Thomas Sailer
 *
 */
/*****************************************************************************/

#include <linux/module.h>
#include <linux/socket.h>
#include <linux/list.h>
#include <linux/vmalloc.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <asm/uaccess.h>
#include <asm/atomic.h>
#include <linux/delay.h>
#include <linux/usb.h>
#include <linux/smp_lock.h>

#include "wavefinder.h"

static int wavefinder_prepare_urb(pwavefinder_t, struct urb*);
static void wavefinder_iso_complete(struct urb*, struct pt_regs*);
static int wavefinder_free_buffers(pwavefinder_t);
static int wavefinder_alloc_buffers(pwavefinder_t);
static int wavefinder_stop(pwavefinder_t);
static int wavefinder_start_receive(pwavefinder_t);
static ssize_t wavefinder_read(struct file *, char __user *, size_t, loff_t *);
static int wavefinder_open(struct inode *, struct file *);
static long wavefinder_ioctl(struct file *, unsigned int, unsigned long);
static int wavefinder_probe(struct usb_interface *, const struct usb_device_id *);
static void wavefinder_disconnect(struct usb_interface *);

/*
 * Version Information
 */
#define DRIVER_VERSION "v1.01"
#define DRIVER_AUTHOR "David Crawley degressive@yahoo.co.uk"
#define DRIVER_DESC "WaveFinder Driver for Linux (c)2005-2010"

#ifdef dbg
#undef dbg
#define dbg(format, arg...) printk(KERN_ALERT format "\n", ## arg)
#endif

#ifdef DEBUG 
static void dump_urb(struct  urb* purb)
{
	int i, j;

	//dbg("urb                   :%p", purb);
	//dbg("dev                   :%p", purb->dev);
	//dbg("pipe                  :%08X", purb->pipe);
	//dbg("status                :%d", purb->status);
	//dbg("transfer_flags        :%08X", purb->transfer_flags);
	//dbg("transfer_buffer       :%p", purb->transfer_buffer);
	//dbg("transfer_buffer_length:%d", purb->transfer_buffer_length);
	//dbg("actual_length         :%d", purb->actual_length);
	//dbg("setup_packet          :%p", purb->setup_packet);
	//dbg("start_frame           :%d", purb->start_frame);
	//dbg("number_of_packets     :%d", purb->number_of_packets);
	//dbg("interval              :%d", purb->interval);
	//dbg("error_count           :%d", purb->error_count);
	//dbg("context               :%p", purb->context);
	//dbg("complete              :%p", purb->complete);
	for (i=0; i < purb->number_of_packets; i++) {
		//dbg("iso offset %d = %d",i,purb->iso_frame_desc[i].offset);
		//dbg("iso length %d = %d",i,purb->iso_frame_desc[i].length);
		//dbg("iso status %d = %d",i,purb->iso_frame_desc[i].status);
		dbg("packet %d (off=%d,len=%d,alen=%d,st=%d) iso bytes %02X %02X %02X %02X",
		    i,
		    purb->iso_frame_desc[i].offset,
		    purb->iso_frame_desc[i].length,
		    purb->iso_frame_desc[i].actual_length,
		    purb->iso_frame_desc[i].status,
		    *((unsigned char*)purb->transfer_buffer+purb->iso_frame_desc[i].offset),
		    *((unsigned char*)purb->transfer_buffer+purb->iso_frame_desc[i].offset+1),
		    *((unsigned char*)purb->transfer_buffer+purb->iso_frame_desc[i].offset+2),
		    *((unsigned char*)purb->transfer_buffer+purb->iso_frame_desc[i].offset+3));
	}
}
#endif

static struct usb_device_id wavefinder_ids [] = {
	{ USB_DEVICE(0x09cd, 0x2001) },
	{ }						/* Terminating entry */
};

MODULE_DEVICE_TABLE(usb, wavefinder_ids);

static struct usb_driver wavefinder_driver =
{
        .name =         "wavefinder",
	.probe =	wavefinder_probe,
	.disconnect =	wavefinder_disconnect,
	.id_table =	wavefinder_ids,
};


static int wavefinder_prepare_urb(pwavefinder_t s, struct urb *purb)
{
	int j, offs;
	int packets = _ISOPIPESIZE / s->pipesize;

	//dbg("wavefinder_prepare_urb");

	for (j = offs = 0; j < packets; j++, offs += s->pipesize) {
		purb->iso_frame_desc[j].offset = offs;
		purb->iso_frame_desc[j].length = s->pipesize;
	}
	purb->interval = 1;
	return 0;
}

static void wavefinder_iso_complete(struct urb *purb, struct pt_regs *regs)
{
	pwavefinder_t s = purb->context;
	int i,j, subret, pkts = 0;
	int len;
	char *bptr, *uptr;
	const unsigned char vhdr[] = {0x0c, 0x62};  /* Valid packets start like this */ 

	//dump_urb(purb);

	j=0;
	while ((purb != s->wfurb[j]) && (j < NURBS))
		j++;

	if (j < NURBS)
		s->running[j] = 0;
	else
		dbg("wavefinder_iso_complete: invalid urb %d", j);

	/* process if URB was not killed */
	if (purb->status != -ENOENT) {
		//dbg("wavefinder_iso_complete urb %d",j);
		for (i = 0; i < purb->number_of_packets; i++) {
			uptr = purb->transfer_buffer+purb->iso_frame_desc[i].offset;
			if (!purb->iso_frame_desc[i].status) {
				len = purb->iso_frame_desc[i].actual_length;
				if (len <= s->pipesize) {
					/*  Won't copy bad/duplicate packets */
					if (!strncmp(vhdr, uptr, 2) && strncmp(uptr, s->prev_h, 12)) {
						bptr = s->recbuf + (s->count % (_ISOPIPESIZE * NURBS));
						memcpy(bptr, uptr, len);
						memcpy(s->prev_h, bptr, 12);
						s->count += len;
						pkts++;
					}
				} else
					dbg("wavefinder_iso_complete: invalid len %d", len);
			} else
				dbg("wavefinder_iso_complete: corrupted packet status: %d", purb->iso_frame_desc[i].status);
			
		}
	}
	
	wavefinder_prepare_urb(s, s->wfurb[j]);
	if ((subret = usb_submit_urb(s->wfurb[j], GFP_ATOMIC)) == 0) {
		s->running[j] = 1;
	} else
		dbg("wavefinder_iso_complete: usb_submit_urb failed %d\n", subret);
	
	if (atomic_dec_and_test(&s->pending_io) && !s->remove_pending && s->state != _stopped) {
		s->overruns++;
		err("overrun (%d)", s->overruns);
	}

	wake_up(&s->wait);
}


static int wavefinder_free_buffers(pwavefinder_t s)
{
	int i;

	dbg("wavefinder_free_buffers");

	for (i=0; i < NURBS; i++) {
		if (s->allocated[i]) {
			//dbg("kfree(s->wfurb[%d]->transfer_buffer)",i);
			kfree(s->wfurb[i]->transfer_buffer);
			//dbg("usb_free_urb(s->wfurb[%d])",i);
			usb_free_urb(s->wfurb[i]);
			s->allocated[i] = 0;
		}
	}

	if (!s->recbuf) {
		//dbg("kfree(s->recbuf)");
		kfree(s->recbuf);
	}

	return 0;
}

static int wavefinder_alloc_buffers(pwavefinder_t s)
{
	unsigned int pipe = usb_rcvisocpipe(s->usbdev, _WAVEFINDER_ISOPIPE);
	int pipesize = usb_maxpacket(s->usbdev, pipe, usb_pipeout(pipe));
	int packets;
	int transfer_buffer_length;
	int i,j;
	
	dbg("wavefinder_alloc_buffers");

	if (!pipesize)	{
		err("ISO-pipe has size 0!!");
		return -ENOMEM;
	}
	packets = _ISOPIPESIZE / pipesize;
	transfer_buffer_length = packets * pipesize;
	
	/*dbg("wavefinder_alloc_buffers pipesize:%d packets:%d transfer_buffer_len:%d",
	  pipesize, packets, transfer_buffer_length);*/

	s->pipesize = pipesize;

	for (i=0; i < NURBS; i++) {
		s->wfurb[i] = usb_alloc_urb(packets,GFP_KERNEL);
		if (!s->wfurb[i]) {
			err("usb_alloc_urb == NULL");
			goto err;
		}
		s->wfurb[i]->transfer_buffer = kmalloc(transfer_buffer_length, GFP_KERNEL);
		if (!s->wfurb[i]->transfer_buffer) {
			kfree(s->wfurb[i]->transfer_buffer);
			err("kmalloc(%d)==NULL", transfer_buffer_length);
			goto err;
		}

		s->wfurb[i]->transfer_buffer_length = transfer_buffer_length;
		s->wfurb[i]->number_of_packets = packets;
		s->wfurb[i]->interval = 1;
		s->wfurb[i]->complete = (usb_complete_t)wavefinder_iso_complete;
		s->wfurb[i]->context = s;
		s->wfurb[i]->dev = s->usbdev;
		s->wfurb[i]->pipe = pipe;
		s->wfurb[i]->transfer_flags = URB_ISO_ASAP;

		for (j = 0; j < packets; j++) {
			s->wfurb[i]->iso_frame_desc[j].offset = j * pipesize;
			s->wfurb[i]->iso_frame_desc[j].length = pipesize;
		}
		s->allocated[i] = 1;
		s->running[i] = 0;
	}
	
	s->recbuf = kmalloc(transfer_buffer_length*NURBS, GFP_KERNEL);
	
	if (!s->recbuf) {
		kfree(s->recbuf);
		err("kmalloc(%d)==NULL (recbuf)", transfer_buffer_length*NURBS);
		goto err;
	}

	return 0;

err:
	wavefinder_free_buffers(s);
	return -ENOMEM;
}


static int wavefinder_stop(pwavefinder_t s)
{
        int i;

	dbg("wavefinder_stop");

	s->state = _stopped;
	for (i=0; i < NURBS; i++) {
		if (s->running[i]) {
			usb_kill_urb(s->wfurb[i]);
			s->running[i] = 0;
		}
	}

	s->pending_io.counter = 0;
	return 0;
}


static int wavefinder_start_receive(pwavefinder_t s)
{
        int i, subret = 0;
        int allocated = 0;

	//dbg("wavefinder_start_receive");

        for (i=0; i < NURBS; i++)
                allocated += s->allocated[i];

	if (allocated != NURBS && s->state != _started) {
		if (wavefinder_alloc_buffers(s) < 0)
			return -ENOMEM;
		//wavefinder_stop(s);
		s->state = _started;
		s->readptr = 0;

		for (i=0; i < NURBS; i++) {
			wavefinder_prepare_urb(s, s->wfurb[i]);
			//dbg("wavefinder_start_receive: submitting urb %d",i);
			if (s->running[i] == 0 && (subret = usb_submit_urb(s->wfurb[i], GFP_KERNEL)) == 0)
				s->running[i] = 1;
			else {
				s->running[i] = 0;
				printk(KERN_DEBUG "wavefinder_start_receive: usb_submit_urb failed %d\n", subret);
			}
		}
	}

	return 0;
}


static ssize_t wavefinder_read(struct file *file, char __user *buf, size_t count, loff_t *ppos)
{
        pwavefinder_t s = (pwavefinder_t) file->private_data;
	DECLARE_WAITQUEUE(wait, current);
	ssize_t ret = 0;
	unsigned long flags;
	int cnt = 0, err, rem;

        /* dbg("wavefinder_read"); */

	if (*ppos)
		return -ESPIPE;

	if (!access_ok(VERIFY_WRITE, buf, count))
		return -EFAULT;

	add_wait_queue(&s->wait, &wait);

	while (count > 0) {
		spin_lock_irqsave(&s->lock, flags);
		rem = s->count - s->readptr;
		if (count >= rem)
			cnt = rem;
		else
			cnt = count;
		/* set task state early to avoid wakeup races */
		if (cnt <= 0)
			__set_current_state(TASK_INTERRUPTIBLE);
		spin_unlock_irqrestore(&s->lock, flags);

		if (cnt > count)
			cnt = count;
		if (cnt <= 0) {
			if (wavefinder_start_receive(s)) {
				if (!ret)
					ret = -ENODEV;
				break;
			}
			if (file->f_flags & O_NONBLOCK) {
				if (!ret)
					ret = -EAGAIN;
				break;
			}
			schedule();
			if (signal_pending(current)) {
				if (!ret)
					ret = -ERESTARTSYS;
				break;
			}
			continue;
		}

		//dbg("wavefinder_read: copy_to_user %d bytes s->readptr=%d s->count=%d",cnt,s->readptr,s->count);
		if ((err = copy_to_user(buf, s->recbuf + s->readptr, cnt))) {
			dbg("copy_to_user err = %d",err);
			if (!ret)
				ret = -EFAULT;
			goto err;
		}

		s->readptr += cnt;
		count -= cnt;
		buf += cnt;
		ret += cnt;

		if (s->readptr == s->count) {
			s->readptr = 0;
			spin_lock_irqsave(&s->lock, flags);	
			s->count = 0;
			spin_unlock_irqrestore(&s->lock, flags);
		}
	}
	__set_current_state(TASK_RUNNING);
	remove_wait_queue(&s->wait, &wait);

err:
	return ret;
}


static int wavefinder_open(struct inode *inode, struct file *file)
{
	int i,  devnum;
	pwavefinder_t s;
	struct usb_interface *interface;

	dbg("wavefinder_open");

	devnum = iminor(inode);

	interface = usb_find_interface (&wavefinder_driver, devnum);
	dbg("wavefinder_open: devnum = %d", devnum);
	s = usb_get_intfdata(interface);

	down(&s->mutex);
	if (!s->usbdev)	{
		up(&s->mutex);
		return -ENODEV;		
	}
		
	if (s->opened) {
		up(&s->mutex);
		return -EBUSY;		
	}
	
	s->opened = 1;
	up(&s->mutex);

	s->count = 0;
	s->readptr = 0;

	for (i = 0; i < NURBS; i++)
		s->allocated[i] = 0;
	s->recbuf = 0;


	file->f_pos = 0;
	file->private_data = s;

	return 0;
}

static int wavefinder_release(struct inode *inode, struct file *file)
{
	pwavefinder_t s = (pwavefinder_t) file->private_data;

	dbg("wavefinder_release");

	down(&s->mutex);
	wavefinder_stop(s);
	wavefinder_free_buffers(s);
	up(&s->mutex);

	wake_up(&s->remove_ok);

	s->opened = 0;
	return 0;
}


static long wavefinder_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	pwavefinder_t s = (pwavefinder_t) file->private_data;
	ctrl_transfer_t *ctrl;
	int ret = 0;

	/* dbg("wavefinder_ioctl"); */

	if (s->remove_pending)
		return -EIO;

	down(&s->mutex);

	if (!s->usbdev) {
		up(&s->mutex);
		return -EIO;
	}

	switch (cmd) {

	case IOCTL_WAVEFINDER_CMSGW:
		ctrl = (ctrl_transfer_t*) kmalloc(sizeof(ctrl_transfer_t), GFP_KERNEL);

		if (!ctrl) {
			ret = -ENOMEM;
			break;
		}

		if (copy_from_user(ctrl, (void *) arg, sizeof(ctrl_transfer_t))) {
			ret = -EFAULT;
			kfree(ctrl);
			break;
		}

		ret=usb_control_msg(s->usbdev, usb_sndctrlpipe( s->usbdev, 0 ), 
				    ctrl->setup.bRequest, ctrl->setup.bRequestType,
				    ctrl->setup.wValue,ctrl->setup.wIndex,
				    ctrl->data, ctrl->size,HZ);
		kfree(ctrl);
		break;


	default:
		ret = -ENOIOCTLCMD;
		break;
	}
	up(&s->mutex);
	return ret;
}

static struct file_operations wavefinder_fops =
{
	.owner =	  THIS_MODULE,
	.read =		  wavefinder_read,
	.unlocked_ioctl = wavefinder_ioctl,
	.open =	          wavefinder_open,
	.release =	  wavefinder_release,
};

static struct usb_class_driver wavefinder_class = {
	.name =		"usb/wavefinder%d",
	.fops =		&wavefinder_fops,
	.minor_base =	WAVEFINDER_MINOR,
};


static int wavefinder_probe(struct usb_interface *intf,
			    const struct usb_device_id *id)
{
        struct usb_device *usbdev = interface_to_usbdev(intf);
	int retval;
	pwavefinder_t s;

	/* intf->minor only becomes valid after usb_register_dev() */
	retval = usb_register_dev(intf, &wavefinder_class);

	s = (wavefinder_t*)kmalloc(sizeof(wavefinder_t), GFP_KERNEL);
	if (!s)
		return -ENOMEM;
	memset(s,0,sizeof(wavefinder_t));
	sema_init (&s->mutex,1);
	init_waitqueue_head (&s->wait);
	init_waitqueue_head (&s->remove_ok);
	spin_lock_init (&s->lock);
		
	s->remove_pending = 0;
	s->usbdev = usbdev;
	s->devnum = intf->minor;
	s->state = _stopped;

	dbg("wavefinder: probe: vendor id 0x%x, device id 0x%x ifnum:%d intf->minor:%d s=%ud wavefinder_t=%d",
	    usbdev->descriptor.idVendor, usbdev->descriptor.idProduct, intf->altsetting->desc.bInterfaceNumber,intf->minor,(unsigned int)s,(int)(sizeof(wavefinder_t)));

	if (intf->altsetting->desc.bInterfaceNumber != _WAVEFINDER_IF)
	        goto reject;
	
	down(&s->mutex);

	if (usbdev == (struct usb_device*)NULL)
		goto reject;

	if (usb_reset_configuration(usbdev) < 0) {
		err("reset_configuration failed");
		goto reject;
	} 
	dbg("bound to interface: %d", intf->altsetting->desc.bInterfaceNumber);
	usb_set_intfdata(intf, s);
	up(&s->mutex);

	dbg("wavefinder: probe: intf->minor:%d",intf->minor);

	if (retval) {
		usb_set_intfdata(intf, NULL);
		return -ENOMEM;
	}

	return 0;

reject:
	up(&s->mutex);
	s->usbdev = NULL;
	return -ENODEV;
}

static void wavefinder_disconnect(struct usb_interface *intf)
{
        wait_queue_t __wait;
	pwavefinder_t s = usb_get_intfdata(intf);

	dbg("wavefinder_disconnect");

	init_waitqueue_entry(&__wait, current);

	usb_set_intfdata(intf, NULL);
	if (s) {
		usb_deregister_dev(intf, &wavefinder_class);
		s->remove_pending = 1;
		wake_up(&s->wait);
		add_wait_queue(&s->remove_ok, &__wait);
		set_current_state(TASK_UNINTERRUPTIBLE);
		if (s->state == _started)
			schedule();
		set_current_state(TASK_RUNNING);
		remove_wait_queue(&s->remove_ok, &__wait);
		s->usbdev = NULL;
		s->overruns = 0;
	}
}

static int __init wavefinder_init(void)
{
        int retval;

	dbg("wavefinder_init");

	/* register misc device */
	retval = usb_register(&wavefinder_driver);
	if (retval)
		goto out;

	dbg("wavefinder_init: driver registered");
	dbg(DRIVER_VERSION ": " DRIVER_DESC);

out:
	return retval;
}

static void __exit wavefinder_cleanup(void)
{
	dbg("wavefinder_cleanup");

	usb_deregister(&wavefinder_driver);

	dbg("wavefinder_cleanup finished");
}


MODULE_AUTHOR( DRIVER_AUTHOR );
MODULE_DESCRIPTION( DRIVER_DESC );
MODULE_LICENSE("GPL");

module_init(wavefinder_init);
module_exit(wavefinder_cleanup);
