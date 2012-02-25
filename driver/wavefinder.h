/* -*- linux-c -*- ***********************************************************/
/*
 *      wavefinder.h  --  part of the Psion WaveFinder driver for Linux 2.6 kernel
 *
 *      Copyright (C) 2005 David Crawley <degressive@yahoo.co.uk>
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
 *      along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
/*****************************************************************************/

typedef struct {
	struct usb_ctrlrequest setup;
	unsigned int size;
	unsigned char data[64];
} ctrl_transfer_t;

#define WAVEFINDER_MINOR 240
#define WAVEFINDER_VERSION 0x1000
#define WAVEFINDER_VENDOR 0x9cd
#define WAVEFINDER_PRODUCT 0x2001
#define IOCTL_WAVEFINDER_CMSGW           _IOWR('d', 0x30, ctrl_transfer_t*)

#define NURBS 2

#ifdef __KERNEL__

typedef enum { _stopped=0, _started } driver_state_t;

typedef struct
{
        struct semaphore mutex;
	struct usb_device *usbdev;
	wait_queue_head_t wait;
	wait_queue_head_t remove_ok;
	spinlock_t lock;
	atomic_t pending_io;
	driver_state_t state;
	int remove_pending;
	unsigned int overruns;
	int readptr;
	int opened;
        int devnum;
        int count; 
        int pipesize;                     /* Calculate this once */
        struct urb *wfurb[NURBS];
 	unsigned int last_order;
        int allocated[NURBS];
        int running[NURBS];
	char *recbuf;
	unsigned char prev_h[12];         /* Stores previous 12-byte packet header */
} wavefinder_t, *pwavefinder_t;

typedef struct 
{
	pwavefinder_t s;
	struct urb *purb;
	struct list_head buff_list;
} buff_t,*pbuff_t;


#define _WAVEFINDER_IF 0
#define _WAVEFINDER_ISOPIPE 0x81
#define _ISOPIPESIZE	16768

#endif
/* KERNEL */

