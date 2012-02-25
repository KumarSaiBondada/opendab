/*
    wfinit.c

    Copyright (C) 2005, 2006, 2007, 2008 David Crawley

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
** Initialize the WaveFinder.
** Send the necessary control words, boot DSPs etc.
*/

#include "opendab.h"

/*
** Boot the WaveFinder DSPs.
**
** Each of the two TMS320VC5402 DSPs are booted via their Host Port Interfaces (HPI-8)
** see chapter 4 of Vol.5 (Enhanced Peripherals) of TI's TMS320C54x DSP Reference Set
** (Document SPRU302) from www.ti.com.
**
** This requires the two files rsDSPa.bin and rsDSPb.bin which contain executable code.
** TODO: It would be interesting to attempt to write GPL'ed Open Source versions.
** TODO: Maybe these filenames should be defined in a configuration file.
**
** Code is loaded via the HPI and then the entry point address is
** loaded into address 0x007f - changing the contents of this address
** from zero causes execution to start at the address specified by the
** new contents (see TI document SPRA618A).
**
** Locations of the HPI registers in the SL11R address space are defined in wfsl11r.h
**
** Note that the TMS320VC5402 has an 8-bit wide HPI (HPI-8) so 16-bit words have to
** be transferred a byte at a time.  The WaveFinder firmware dumbly writes anything it
** receives to the HPI. So, in order to send one 16-bit word, we actually have to send
** two 16-bit words. If the word we want to send is 0xmmll, we need to send 0xmm00 0xll00
**
** Also: 1. The first word of the rsDSP[ab].bin files is the entry point.
**       2. Only bytes from offset 0x80 to 0x2000 are uploaded from each file.
*/
int wf_boot_dsps(int fd)
{
	/* The other software uses this length (16 bit words) for the data transfer stage */ 
#define USBDATALEN 31

	FILE *fp;
	char *filename[] = {"rsDSPb.bin","rsDSPa.bin"};
	/*  unsigned short ctrlreg[2] = {HPIC_B, HPIC_A}; */
	unsigned short addrreg[2] = {HPIA_B, HPIA_A};
	unsigned short datareg[2] = {HPID_B, HPID_A};
	unsigned short entry_pt[2];
	unsigned short rbuf[3];
	unsigned char ubuf[USBDATALEN*2];
	struct stat sbuf;
	unsigned char *cbuf;
	int i,j,remain, left, frames;

	/* This loads 0x0000 into DSP B at address 0x00e0
	   But why ?  Not even the HPI control reg has been loaded yet
	   TODO: Check the reason for this */

	rbuf[0] = HPIA_B; /* Load HPI address register */
	rbuf[1] = 0x00e0;
	rbuf[2] = 0x0000;
	wf_sendmem(fd, 0, 0, (unsigned char*)&rbuf, 6);

	rbuf[0] = HPID_B; /* Load HPI data register */
	rbuf[1] = 0x0000;
	rbuf[2] = 0x0000;
	wf_sendmem(fd, 0, 0, (unsigned char*)&rbuf, 6);  

	rbuf[0] = HPIC_B; /* Load HPI control register */
	rbuf[1] = 0x0001;
	rbuf[2] = 0x0001;
	wf_sendmem(fd, 0, 0, (unsigned char*)&rbuf, 6);

	rbuf[0] = HPIC_A; /* Load HPI control register */
	rbuf[1] = 0x0001;
	rbuf[2] = 0x0001;
	wf_sendmem(fd, 0, 0, (unsigned char*)&rbuf, 6);

	/* Open, read and upload the DSP code files */

	for (i = 0; i < 2; i++) {
		if ((fp = fopen(filename[i], "rb")) == (FILE *)NULL) {
			fprintf(stderr,"Couldn't open %s\n",filename[i]);
			wf_close(fd);
		}
		stat(filename[i], &sbuf);
		/* printf("File: %s is %d bytes\n",filename[i],(int)sbuf.st_size); */
		if ((cbuf = (unsigned char*)malloc((size_t)sbuf.st_size)) == (unsigned char*)NULL) {
			fprintf(stderr,"malloc failed for %s buffer\n",filename[i]);
			wf_close(fd);
		}      
		if ((j=fread(cbuf, 1, sbuf.st_size, fp)) != sbuf.st_size) {
			fprintf(stderr,"fread failed for %s (read %d bytes of an expected %d)\n",filename[i],j,(int)sbuf.st_size);
			wf_close(fd);
		}

		rbuf[0] = addrreg[i]; /* Load HPI address register - actually at 0x0080 because of inc. */
		rbuf[1] = 0x007f;
		rbuf[2] = 0x0000;
		wf_sendmem(fd, 0, 0, (unsigned char*)&rbuf, 6);

		frames = 0;
		memset(ubuf, 0, USBDATALEN*2);  /* zero buffer - unnecessary but less confusing to debug */    
		ubuf[0] = datareg[i] & 0xff;
		ubuf[1] = (datareg[i] >> 8) & 0xff;
		remain = 0x2000 - 0x80 + 1;         /* bytes to transfer */
		while (remain > 0) {
			if (remain >= USBDATALEN) {
				for (j=2; j < USBDATALEN*2; j+=2)
					ubuf[j] = *(cbuf + 0x2001 - remain--);
				wf_sendmem(fd, datareg[i], 0, (unsigned char*)&ubuf, USBDATALEN*2);
				frames++;
			} else {
				left = remain*2;
				for (j=0; j < left; j+=2)
					ubuf[j+2] = *(cbuf + 0x2000 - remain--);
				wf_sendmem(fd, datareg[i], 0, (unsigned char*)&ubuf, left);
				frames++;
			}
		}
		/* printf("\n Frames = %d\n",frames); */
		entry_pt[i] = *cbuf | (*(cbuf + 1) << 8);
		free(cbuf);
	}

	/* With the code uploaded, load address 0x007f
	   of each DSP with the appropriate entry point to begin execution */

	rbuf[0] = HPIA_A; /* Load HPI address register for DSP A */
	rbuf[1] = 0x007e; /* = 0x007f after increment */
	rbuf[2] = 0x0000;
	wf_sendmem(fd, 0, 0, (unsigned char*)&rbuf, 6);

	rbuf[0] = HPIA_B; /* Load HPI address register for DSP B */
	rbuf[1] = 0x007e; /* = 0x007f after increment */
	rbuf[2] = 0x0000;
	wf_sendmem(fd, 0, 0, (unsigned char*)&rbuf, 6);

	rbuf[0] = HPID_A; /* Load HPI data register for DSP A */
	rbuf[1] = entry_pt[1] & 0xff;
	rbuf[2] = (entry_pt[1] & 0xff00) >> 8;
	wf_sendmem(fd, 0, 0, (unsigned char*)&rbuf, 6);  

	rbuf[0] = HPID_B; /* Load HPI data register for DSP B */
	rbuf[1] = entry_pt[0] & 0xff;;
	rbuf[2] = (entry_pt[0] & 0xff00) >> 8;
	wf_sendmem(fd, 0, 0, (unsigned char*)&rbuf, 6);

	/* This also gets sent. TODO: why ? */
	rbuf[0] = HPIA_B; /* Load HPI address register for DSP B */
	rbuf[1] = 0x00ff;
	rbuf[2] = 0x003e;
	wf_sendmem(fd, 0, 0, (unsigned char*)&rbuf, 6);  
	rbuf[0] = HPID_B; /* Load HPI data register for DSP B */
	rbuf[1] = 0x00;
	rbuf[2] = 0x00;
	wf_sendmem(fd, 0, 0, (unsigned char*)&rbuf, 6);
	rbuf[0] = HPID_B; /* Load HPI data register for DSP B */
	rbuf[1] = 0x00;
	rbuf[2] = 0x00;
	wf_sendmem(fd, 0, 0, (unsigned char*)&rbuf, 6);
	rbuf[0] = HPIA_A; /* Load HPI address register for DSP A */
	rbuf[1] = 0x00ff;
	rbuf[2] = 0x001f;
	wf_sendmem(fd, 0, 0, (unsigned char*)&rbuf, 6);  
	rbuf[0] = HPIA_B; /* Load HPI address register for DSP B */
	rbuf[1] = 0xff;
	rbuf[2] = 0x1f;
	wf_sendmem(fd, 0, 0, (unsigned char*)&rbuf, 6);

	return 0;
}

/*
** TODO: what are these messages for ?
** This isn't well written, but needs to be replaced anyway.
** Incidentally, USBsnoop and Dabble don't seem to agree about
** what gets sent - does the w!nd*ws driver do something odd ?
** In fact, it doesn't seem to matter what data is sent in these
** 64 bytes - hence setting it to zero.
*/
int wf_req1req2(int fd, int reqnum, int msgnum)
{
	/* unsigned char r0[] = {0xff,0xff,0xff,0xff,0x00,0x00,0x00,0x00,
			      0x9d,0x3f,0x1b,0xff,0xd4,0xeb,0x7c,0xdd,
			      0x05,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
			      0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
			      0x80,0xd4,0x92,0xda,0xe0,0x80,0x92,0xda,
			      0x02,0x80,0x00,0x00,0x00,0x00,0x00,0x00,
			      0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
			      0x00,0x00,0x00,0x00,0x27,0x4a,0x1b,0xff};

	unsigned char r3[] = {0x93,0x02,0x00,0x00,0x32,0x15,0x03,0xc0,
			      0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
			      0xb0,0x71,0x02,0xc0,0x00,0x00,0x00,0x00,
			      0x00,0x02,0x00,0x00,0x93,0x02,0x00,0x00,
			      0x87,0x15,0x03,0xc0,0x31,0x0e,0x03,0xc0,
			      0xfc,0x80,0x92,0xda,0x95,0xf1,0x02,0xc0,
			      0x2a,0xba,0x02,0xc0,0xfc,0x80,0x92,0xda,
			      0x05,0x37,0x1b,0xff,0x0d,0x37,0x1b,0xff}; */

        unsigned char r2[] = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
			      0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
			      0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
			      0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
			      0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
			      0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
			      0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
			      0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};

	/* unsigned char *p = r0; */
	unsigned char *p = r2;

	switch (msgnum) {
	case 0:
		/*p = r0;*/
		p = r2;
		break;
	case 1:
		/*p = r3;*/
		p = r2;
		break;
	}

	if (reqnum == 1)
		wf_r1_msg(fd, p);
	else
		wf_r2_msg(fd, p);

	return 0;
}

/*
** Initialize
*/
int wf_init(int fd, double freq)
{
	/* Initialize various SL11R registers - see wfsl11r.h */
	/* Much of this is concerned with the PWM channels which control
	   the coloured LEDs. */
	wf_req1req2(fd, 2, 0);
	wf_mem_write(fd, PWMCTRLREG, 0);
	wf_mem_write(fd, PWMMAXCNT, 0x03ff);

	wf_mem_write(fd, PWMCH0STRT, 0);
	wf_mem_write(fd, PWMCH0STOP, 0);

	wf_mem_write(fd, PWMCH1STRT, 0);
	wf_mem_write(fd, PWMCH1STOP, 0);

	wf_mem_write(fd, PWMCYCCNT, 0x03ff);

	wf_mem_write(fd, PWMCH2STRT, 0);
	wf_mem_write(fd, PWMCH2STOP, 0);

	wf_mem_write(fd, PWMCH3STRT, 0);
	wf_mem_write(fd, PWMCH3STOP, 0);

	wf_mem_write(fd, PWMCH0STRT, 0);
	wf_mem_write(fd, PWMCH0STOP, 0x02ff);

	wf_mem_write(fd, PWMCH1STOP, 0x02ff);

	wf_mem_write(fd, PWMCTRLREG, 0x800f);
	wf_mem_write(fd, IOCTRLREG1, 0x3de0);
	wf_mem_write(fd, UNK0XC120, 0);        /* TODO: work out what's at 0xc120 */
	wf_sleep(100000);
	wf_mem_write(fd, UNK0XC120, 0xffff);
	wf_mem_write(fd, OUTREG1, 0x3800);     /* TODO: work out what each bit controls */
	wf_mem_write(fd, OUTREG0, 0x0000);
	wf_mem_write(fd, OUTREG1, 0x3000);
	wf_mem_write(fd, OUTREG1, 0x3800);
	wf_boot_dsps(fd);
	wf_mem_write(fd, OUTREG0, 0x1000);     /* TODO: work out what each bit controls */
	wf_leds(fd,0x3ff, 0x180, 0x3ff); /* Green LED on as simple indicator */
	wf_tune(fd, freq);
	wf_sleep(400000);
	wf_timing(fd, 0);
	wf_sleep(4000);
	wf_timing(fd, 1);
	wf_sleep(4000);
	wf_timing(fd, 1);
	wf_sleep(4000);
	wf_timing(fd, 2);
	wf_sleep(50000);
	wf_mem_write(fd, DACVALUE, 0x5330);
	wf_mem_write(fd, DACVALUE, 0x5330);
	wf_sleep(77000);
	/* The next control message causes the WaveFinder to start sending
	   isochronous data */
	wf_req1req2(fd, 1, 1);
	/* wf_read(fd, of, &pkts); */
	wf_mem_write(fd, PWMCTRLREG, 0x800f);
	wf_timing(fd, 1);
	wf_timing(fd, 2);
	wf_timing(fd, 1);
	wf_timing(fd, 3);
	wf_tune(fd, freq);
	wf_sleep(200000);
	wf_timing(fd, 4);
	wf_tune(fd, freq);
	wf_sleep(200000);
	wf_tune(fd, freq);
	wf_sleep(200000);
	wf_mem_write(fd, DACVALUE, 0x5330);
        return 0;
}
