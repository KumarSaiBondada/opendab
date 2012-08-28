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

#include "opendab.h"

/* DEBUG:
** 0 - no info
** 1 - print messages and timestamps
*/
#define DEBUG 0

#ifndef __APPLE__

#define USB_TYPE_VENDOR (0x02 << 5)

struct wavefinder *wf_open(char *devname)
{
        int fd;
        struct wavefinder *wf;

	fd = open(devname,O_RDWR);
        if (fd == -1)
                return NULL;

        if ((wf = malloc(sizeof (struct wavefinder))) == NULL)
                return NULL;

        wf->fd = fd;
        return wf;
}

int wf_close(struct wavefinder *wf)
{
	wf_leds_off(wf);
	wf_mem_write(wf, OUTREG1, 0x2000);
	close(wf->fd);
	return 0;
}

/*
** Read incoming data from the WaveFinder.
**
** There is one isochronous stream which contains
** (at least) two types of data:
**
** (i) Phase Reference Symbol (PRS) used for synchronization
** (ii) QPSK symbols
**
** Data arrives in 524-byte blocks which have a 12-byte
** header
**
** 4 PRS blocks are needed to make a 2048 byte PRS (Mode I)
**
**   0c 62 01 05 36 20 04 03 00 02 00 00 58 c0 66 6a
**   ^^ ^^ 		  ^^    ^^       ^^ 
** Constant	      Block no.	PRS      Data starts here
**
** Other symbols forming the FIC and MSC look like this:
**   0c 62 07 0e f8 20 01 00 80 01 00 00 2f 3e d9 ab
**   ^^ ^^ ^^ ^^                ^^       ^^
**     |   |   Frame no.        QPSK     Data starts here
**     |   Symbol no.
**     |   (0-76)
** Constant
**
*/
int wf_read(struct wavefinder *wf, unsigned char *rdbuf, unsigned int* len)
{
	int l = 0;
	
	while (l == 0)
		l = read(wf->fd,rdbuf,PIPESIZE);

	if (l < 0)
		perror("wf_read");
	else
		*len = l;

	return 0;
}

/*
** USB control message routine - uses kernel driver
** All control messages pass through here, so can be printed for debugging etc.
*/
int wf_usb_ctrl_msg(struct wavefinder *wf, int request,
                    int value, int index, unsigned char *bytes, int size)
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
	ctl.setup.bRequestType = USB_TYPE_VENDOR;
	ctl.setup.bRequest = request;
	ctl.setup.wValue = value;
	ctl.setup.wIndex = index;
	ctl.size = size;

	/* TODO: this copy shouldn't really be needed */
	memcpy(ctl.data, bytes, size*sizeof(unsigned char));
 
	if ((ret=ioctl(wf->fd,IOCTL_WAVEFINDER_CMSGW,&ctl)) == -1) {
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

#endif
