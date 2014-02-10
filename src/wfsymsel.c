/*
    wfsymsel.c

    Copyright (C) 2007  David Crawley

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
** WaveFinder symbol selection
*/

#define _XOPEN_SOURCE 1 /* swab() */

#include <unistd.h>
#include <stdio.h>
#include <string.h>

#include "opendab.h"

/* Convert subchannel start address and size
** to start and end symbol numbers for each of the 
** CIFs. Also calculate subchannel start
** address offset from start of 1st symbol.
*/
int startsym_audio(struct symrange *r, struct audio_subch *s)
{
	int i;

	r->start[0] = s->startaddr/CUSPERSYM + MSCSTART;
	r->end[0] = (s->startaddr + s->subchsz)/CUSPERSYM + MSCSTART;
	r->endcu = (s->startaddr + s->subchsz) % CUSPERSYM;
	for (i=0; i < 3; i++) {
		r->start[i + 1] = r->start[i] + SYMSPERCIF;
		r->end[i + 1] = r->end[i] + SYMSPERCIF;
	}
	r->startcu = s->startaddr % CUSPERSYM;
	r->numsyms = r->end[0] - r->start[0] + 1;
	fprintf(stderr,"subch: %d start[0]=%d end[0]=%d start[1]=%d end[1]=%d start[2]=%d end[2]=%d start[3]=%d end[3]=%d\n",s->subchid, r->start[0],r->end[0],r->start[1],r->end[1],r->start[2],r->end[2],r->start[3],r->end[3]);
	return 0;
}

/* Convert packet address to start and end symbol numbers...
*/
int startsym_data(struct symrange *r, struct data_subch *s)
{
	int i;

	r->start[0] = s->startaddr/CUSPERSYM + MSCSTART;
	r->end[0] = (s->startaddr + s->subchsz)/CUSPERSYM + MSCSTART;
	r->endcu = (s->startaddr + s->subchsz) % CUSPERSYM;
	for (i=0; i < 3; i++) {
		r->start[i + 1] = r->start[i] + SYMSPERCIF;
		r->end[i + 1] = r->end[i] + SYMSPERCIF;
	}
	r->startcu = s->startaddr % CUSPERSYM;
	r->numsyms = r->end[0] - r->start[0] + 1;

	/* fprintf(stderr,"start[0]=%d end[0]=%d start[1]=%d end[1]=%d start[2]=%d end[2]=%d start[3]=%d end[3]=%d\n",r->start[0],r->end[0],r->start[1],r->end[1],r->start[2],r->end[2],r->start[3],r->end[3]); */

	return 0;
}

/*
** Generate bytes to be used in the control messages with
** bRequest == 5 in order to control which symbols the
** WaveFinder sends back.
**
** A series of 5 16-bit words is used. 2 adjacent bits are set
** to select each symbol to be returned.  The leftmost word
** is the least significant.
** So to select symbol no.7 the following bits would be set
** 0000 0110 0000 0000...<8 zero bytes>
** and to select symbol no.8:
** 0000 0011 0000 0000...<8 zero bytes>
** ...symbol no.9:
** 0000 0001 1000 0000...<8 zero bytes>
**
** Values are ORed together to select the set of symbols to be
** returned. Symbols 0 and 1 always appear to be returned and
** usually 2,3, and 4 (which make up the FIC) are always wanted.
** Strangely, setting bit 0 causes symbols 2 and 3 to be returned
** and bit 3 selects symbol 4. This means that, if symbols 2,3,4
** are always wanted, it seems that symbol 6 cannot be selected
** without also selecting symbol 5.
**
** Words are stored in big endian form (ms byte first)
** 
** Use of 64-bit int with overlapping 32-bit int avoids the need
** to propagate shifted bits between words
*/
int wfsymsel(unsigned char *sel, struct symrange *r)
{
	int i, j;
	unsigned int h = 0;
	unsigned long long l = 0ULL, t;

	for (i=0; i < 4; i++)
		for (j=r->start[i]; j < (r->end[i]+1); j++)
			if (j < 63)
				l |= 0xc000000000000000ULL >> (j - 2);
		        else
				h |= 0xc0000000 >> (j - 48 - 2);
	
	swab(&l, &t, sizeof(unsigned long long));
	swab(&h, &j, sizeof(unsigned int));

	memset(sel, 0, 10 * sizeof(unsigned char));
	
	for (i=0; i < 8; i++)
		sel[i] |= (t >> (8 * (7 - i))) & 0xff;

	for (i=0; i < 4; i++)
		sel[i+6] |= (j >> (8 * (3 - i))) & 0xff;

	sel[1] |= 0xf0;   /* Always select symbols 2,3,4 */
	return 0;
}

