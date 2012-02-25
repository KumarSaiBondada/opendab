/*
    opendab.h

    Copyright (C) 2007 David Crawley

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

#define _POSIX_C_SOURCE 199309L
/* #include <linux/usb_ch9.h> */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/ioctl.h>
/* #include <sys/types.h> */
#include <sys/stat.h>
#include <sys/time.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <math.h>
#include <complex.h>
#include <fftw3.h>

#include "wfsl11r.h"


/* TODO: eliminate these declarations - should be unnecessary */
#define USB_TYPE_VENDOR                 (0x02 << 5)
typedef unsigned char __u8;
typedef unsigned short __u16;

struct usb_ctrlrequest {
        __u8 bRequestType;
        __u8 bRequest;
        __u16 wValue;
        __u16 wIndex;
        __u16 wLength;
	} __attribute__ ((packed));
/* TODO: eliminate the above declarations - should be unnecessary */

#include "../driver/wavefinder.h"

#define TRUE -1;
#define FALSE 0;

/* USB Device identifiers for WaveFinder */
#define VENDOR 0x9cd
#define PRODUCT 0x2001

/* WaveFinder bmRequest values */

#define bmRequest1 1
#define bmRequest2 2
#define SLMEM 3
#define WFTUNE 4
#define WFTIMING 5

/* Size of read buffer */
#define PIPESIZE 16768
