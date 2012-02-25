/*
    wftune.c

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
** Generate the necessary bytes to tune the WaveFinder
**
** There are 2 National Semiconductor PLLs
**     - an LMX1511 used for Band III tuning
**     - an LMX2331A used for the L-Band converter.
** 
** In the other software the LMX1511 is always used to tune, with the L-band converter
** operating at a fixed frequency.
**
** When bRequest is 5, the USB data is:
**
** 1st 4 bytes = data to be shifted into the PLL register
** 5th and 6th byte: word = count of the number of bits to shift
** 7th and 8th byte: word = 1 to address the LMX1511, 0 to address the LMX2331A
** 9th and 10th byte: word = 1 selects L-band, 0 selects Band III (tentative)
** 11th and 12th byte apparently unused (tentative).
**
** So tuning the WaveFinder requires 6 messages, 2 to load the LMX1511 and
** 4 to load the LMX2331A.
**
** Messages to tune the LMX2331A contain constant data (currently).
**
** IF is 38.912MHz
** Reference frequency (f_osc) is 16.384MHz
** L-Band converter frequency is 1251.456MHz
** 
** In the other software:
** LMX1511 uses prescaler: 64/65 P=64, R=1024.
** LMX2331A uses RF prescaler: 128/129 P=128, RF R=256, +ve phase, RF N count: A=98, B=152
**     "     "   IF prescaler: 16/17 P=16, IF R=256, +ve phase, IF N count: A=0, B=40.
**
** Data is loaded into a shift register in each of the PLLs via a 3-line serial microwire
** interface by the WaveFinder firmware.
**
** The LMX1511 uses two words - one has 16 bits and the other 19 bits - the LSB determines
** how the shift register contents are interpreted.
**
** The LMX2331A uses four 22-bit words - the two LSBs determine how the shift register
** contents are interpreted.
**
** (I don't know what the IF output from the LMX2331A is used for but it isn't disabled)
**
** For both PLLs (and for the RF and IF sections of the LMX2331A): f_vco = (P*B+A)*f_osc/R
** For the LMX1511, we need to calculate A and B:
** A = ((f_vco/f_osc)*R) mod P
** B = ((f_vco/f_osc)*R) div P
** where f_vco = frequency we want to tune + IF.
**
** Since the LMX2331A is set to a fixed frequency we can just load it with the appropriate
** constant data or (maybe later) we could allow an alternative tuning strategy.
**
** See the National Semiconductor data sheets for more detail.
*/
#include "opendab.h"

/* Maximum Band III frequency (MHz) */
#define MAXFREQIII 240.0

/* For L Band, this (MHz) is subtracted from the input frequency */
#define LBANDOFFSET 1251.456

/* The receiver Intermediate Frequency - WaveFinder uses Hitachi HWSD231 SAW filter */
#define IF 38.912e6

/* Reference frequency */
#define F_OSC 16.384e6

/* LMX1511 R division constant */
#define R_1511 1024.0

/* LMX1511 Prescaler */
#define P_1511 64

/* LMX2331A IF and RF R Counter */
#define R_2331A 256

/* LMX2331A IF N Counter */
#define NIFA_2331A 0
#define NIFB_2331A 40

/* LMX2331A RF N Counter */
#define NRFA_2331A 98
#define NRFB_2331A 152

/* PLL Selection */
#define LMX1511 1
#define LMX2331A 0

unsigned int reverse_bits(unsigned int, unsigned int);
int wf_tune_msg(int, unsigned int, unsigned char, unsigned char, unsigned char);


/*
** Tune the WaveFinder
*/
void wf_tune(int fd, double freq)
{
#define TBUFSIZE 12
	unsigned char lband;
	int f_vco;
	int a_1511, b_1511;
	unsigned int rc;
	double f_vcod;

	lband = FALSE;

	if (freq > MAXFREQIII) {
		lband = TRUE;
		freq = freq - LBANDOFFSET;
	}

	/* *Don't* change the order in which these messages are sent */

	/* Load the RF R counter of the Band L PLL - constants */
	rc = 0x100000 | reverse_bits(R_2331A, 15) << 5 | 0x10;
	wf_tune_msg(fd, rc, 22, LMX2331A, lband);

	/* Load the RF N counter of the Band L PLL - constants */
	rc = 0x300000 | reverse_bits(NRFA_2331A, 7) << 13 | reverse_bits(NRFB_2331A, 11) << 2 | 2;
	wf_tune_msg(fd, rc, 22, LMX2331A, lband);

	/* Load the IF R counter of the Band L PLL - constants */ 
	rc = reverse_bits(R_2331A, 15) << 5 | 0x10;
	wf_tune_msg(fd, rc, 22, LMX2331A, lband);

	/* Load the N counter of the Band III PLL - this does the tuning */
	f_vcod = (freq*1e6 + IF)/(F_OSC/R_1511);
	f_vco = (int)(ceil(f_vcod)); /* TODO: Necessary ?  Seems to be *essential* */

	/* Load the IF N counter of the Band L PLL - constants */
	rc = 0x200000 | reverse_bits(NIFA_2331A, 7) << 13 | reverse_bits(NIFB_2331A, 11) << 2 | 2;
	wf_tune_msg(fd, rc, 22, LMX2331A, lband);

	b_1511 = f_vco / P_1511;
	a_1511 = f_vco % P_1511;
  
	/* Load the R counter and S latch of the Band III PLL - constants */
	rc = 0x8000 | (reverse_bits((int)R_1511, 14)) << 1 | 1;
	wf_tune_msg(fd, rc, 16, LMX1511, lband);

	/* Load the N counter (as A and B counters) of the Band III PLL */
	rc = reverse_bits(a_1511,7) << 11 | reverse_bits(b_1511,11);
	wf_tune_msg(fd, rc, 19, LMX1511, lband);
}

/*
** Reverse the 'len' least sig bits in 'op'
*/ 
unsigned int reverse_bits(unsigned int op, unsigned int len)
{
	int i;
	unsigned int j = 0;

	for (i=0; i < len; i++) {
		if (op & (1 << (len - i - 1)))
			j |= 1 << i;
	}

	return(j);
}
