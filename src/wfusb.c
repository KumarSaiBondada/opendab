/*
    wfusb.c

    Copyright (C) 2005,2006,2007  David Crawley

    This file is part of OpenDAB.

    OpenDAB is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    OpenDAB is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with OpenDAB.  If not, see <http://www.gnu.org/licenses/>.
*/
/*
** USB control message routine - uses kernel driver
** All control messages pass through here, so can be printed for debugging etc.
*/
#include "opendab.h"

/* DEBUG:
** 0 - no info
** 1 - print messages and timestamps
*/
#define DEBUG 0

int wf_usb_ctrl_msg(int fd, int requesttype, int request, int value, int index, unsigned char *bytes, int size)
{
	ctrl_transfer_t ctl;
	int ret;
#if DEBUG > 0
	long secs, usecs;
	int tnow, i;
	static char first = -1;
	static struct timeval itv;
	struct timeval ctv;
#endif
	ctl.setup.bRequestType=requesttype;
	ctl.setup.bRequest=request;
	ctl.setup.wValue=value;
	ctl.setup.wIndex=index;
	ctl.size=size;
	/* TODO: this copy shouldn't really be needed */
	memcpy(ctl.data, bytes, size*sizeof(unsigned char));
 
	if ((ret=ioctl(fd,IOCTL_WAVEFINDER_CMSGW,&ctl)) == -1) {
		perror("wf_usb_ctrl_msg");
		exit(EXIT_FAILURE);
	}
	/* else printf("ioctl: ret = %d\n", ret); */
#if DEBUG > 0
	if (first) {
		gettimeofday(&itv, NULL);
		first = 0;
		tnow = 0;
	} else {
		gettimeofday(&ctv, NULL);
		secs = ctv.tv_sec - itv.tv_sec;
		usecs = ctv.tv_usec - itv.tv_usec;
		tnow = 1000*secs + usecs/1000;
	}
	fprintf(stderr,"[%d ms]\n",tnow);
	/* fprintf(stderr,"wf_usb_ctrl_msg: ctl.setup.bRequestType  = %#0hhx\n", ctl.setup.bRequestType); */
	fprintf(stderr,"Request = %08hhx\n", ctl.setup.bRequest);
	fprintf(stderr,"Value = %08hx\n", ctl.setup.wValue);
	fprintf(stderr,"Index = %08hx\n", ctl.setup.wIndex);
	for (i=0; i < ctl.size; i++)
		fprintf(stderr,"%02x ",ctl.data[i]);
	fprintf(stderr,"\n");

#endif
	return 0;
}

/*
** Convenience function that calls wf_usb_ctrl_msg() with
** bmRequestType = USB_TYPE_VENDOR and bmRequest = SLMEM
*/
int wf_sendmem(int fd, int value, int index, unsigned char *bytes, int size)
{
	/* int i;
	   printf("SLMEM Value=%#hx, Index=%d, Length=%d\n",value, index, size);
	   for (i=0; i < size; i++)
	   printf("%#0hhx ",*(bytes+i));
	   printf("\n");*/
	return(wf_usb_ctrl_msg(fd, USB_TYPE_VENDOR, SLMEM, value, index, bytes, size));
}

/*
** Write a single 16-bit word to a location
** in the WaveFinder's SL11R address space.
** Automatically fills in the data buffer with the values
** from the index and value fields.
**
*/
int wf_mem_write(int fd, unsigned short addr, unsigned short val)
{
	unsigned char bytes[4];

	bytes[0] = (unsigned char)(addr & 0xff);
	bytes[1] = (unsigned char)((addr >> 8) & 0xff);
	bytes[2] = (unsigned char)(val & 0xff);
	bytes[3] = (unsigned char)((val >> 8) & 0xff);

	/* fprintf(stderr,"%x, %x, %x, %x\n", bytes[0], bytes[1], bytes[2], bytes[3]); */

	return(wf_usb_ctrl_msg(fd, USB_TYPE_VENDOR, SLMEM, addr, val, bytes, 4));
}

/*
** Send a tuning message
*/
int wf_tune_msg(int fd, unsigned int reg, unsigned char bits, unsigned char pll, unsigned char lband)
{
#define TBUFSIZE 12
	unsigned char tbuf[TBUFSIZE];

	memset(tbuf, 0, TBUFSIZE);  /* zero buffer */
	tbuf[0] = reg & 0xff;
	tbuf[1] = (reg >> 8) & 0xff;
	tbuf[2] = (reg >> 16) & 0xff;
	tbuf[3] = (reg >> 24) & 0xff;  
	tbuf[4] = bits;     /* no. of reg. bits */
	tbuf[6] = pll;      /* 1 = select LMX1511, 0 = select LMX2331A */
	tbuf[8] = lband ? 1 : 0;
	tbuf[11] = 0x10;  /* TODO: Check what this byte does, if anything */
	return(wf_usb_ctrl_msg(fd, USB_TYPE_VENDOR, WFTUNE, 0, 0, tbuf, TBUFSIZE));
	/* printf("WTUNE Length=%d\n",TBUFSIZE);
	   for (i=0; i < TBUFSIZE; i++)
	   printf("%#0hhx ",*(tbuf+i));
	   printf("\n"); */
}

/* 
** Send a 32 byte timing control/symbol selection message
*/
int wf_timing_msg(int fd, unsigned char* bytes)
{
	return(wf_usb_ctrl_msg(fd, USB_TYPE_VENDOR, WFTIMING, 0, 0, bytes, 32));
}  

/*
** Send a pair of 64 byte request=1, request=2 messages
** TODO: work out what these messages do etc.
*/
int wf_r2_msg(int fd, unsigned char* bytes)
{
	return(wf_usb_ctrl_msg(fd, USB_TYPE_VENDOR, 2, 0, 0x80, bytes, 64));
}

int wf_r1_msg(int fd, unsigned char* bytes)
{
	return(wf_usb_ctrl_msg(fd, USB_TYPE_VENDOR, 1, 0, 0x80, bytes, 64));
}
