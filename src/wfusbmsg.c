#include "opendab.h"

#define DEBUG 0

/*
** Convenience function that calls wf_usb_ctrl_msg() with
** bmRequestType = USB_TYPE_VENDOR and bmRequest = SLMEM
*/
int wf_sendmem(struct wavefinder *wf, int value, int index, unsigned char *bytes, int size)
{
	/* int i;
	   printf("SLMEM Value=%#hx, Index=%d, Length=%d\n",value, index, size);
	   for (i=0; i < size; i++)
	   printf("%#0hhx ",*(bytes+i));
	   printf("\n");*/
	return(wf_usb_ctrl_msg(wf, SLMEM, value, index, bytes, size));
}

/*
** Write a single 16-bit word to a location
** in the WaveFinder's SL11R address space.
** Automatically fills in the data buffer with the values
** from the index and value fields.
**
*/
int wf_mem_write(struct wavefinder *wf, unsigned short addr, unsigned short val)
{
	unsigned char bytes[4];

	bytes[0] = (unsigned char)(addr & 0xff);
	bytes[1] = (unsigned char)((addr >> 8) & 0xff);
	bytes[2] = (unsigned char)(val & 0xff);
	bytes[3] = (unsigned char)((val >> 8) & 0xff);

	/* fprintf(stderr,"%x, %x, %x, %x\n", bytes[0], bytes[1], bytes[2], bytes[3]); */

	return(wf_usb_ctrl_msg(wf, SLMEM, addr, val, bytes, 4));
}

/*
** Send a tuning message
*/
int wf_tune_msg(struct wavefinder *wf, unsigned int reg, unsigned char bits,
                unsigned char pll, unsigned char lband)
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
	return(wf_usb_ctrl_msg(wf, WFTUNE, 0, 0, tbuf, TBUFSIZE));
	/* printf("WTUNE Length=%d\n",TBUFSIZE);
	   for (i=0; i < TBUFSIZE; i++)
	   printf("%#0hhx ",*(tbuf+i));
	   printf("\n"); */
}

/* 
** Send a 32 byte timing control/symbol selection message
*/
int wf_timing_msg(struct wavefinder *wf, unsigned char* bytes)
{
	return(wf_usb_ctrl_msg(wf, WFTIMING, 0, 0, bytes, 32));
}  

/*
** Send a pair of 64 byte request=1, request=2 messages
** TODO: work out what these messages do etc.
*/
int wf_r2_msg(struct wavefinder *wf, unsigned char* bytes)
{
	return(wf_usb_ctrl_msg(wf, 2, 0, 0x80, bytes, 64));
}

int wf_r1_msg(struct wavefinder *wf, unsigned char* bytes)
{
	return(wf_usb_ctrl_msg(wf, 1, 0, 0x80, bytes, 64));
}
