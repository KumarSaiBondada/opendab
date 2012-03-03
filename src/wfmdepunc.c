/*
    wfmdepunc.c

    Copyright (C) 2007,2008 David Crawley

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
** MSC UEP and EEP depuncturing
*/
#include <stdio.h>
#include "wfic/wfic.h"
#include "opendab.h"
#include "prot.h" 

#define BLKSIZE 128

/* Viterbi symbol values 0->127 1->129 erasure->128 */
#define OFFSET 128

int uep_depuncture(unsigned char *obuf, unsigned char *inbuf, struct audio_subch *s, int* len)
{
	int i, j, k, indx;
	const struct uepprof p = ueptable[s->uep_indx];

	j = 0;
	k = 0;
	for (indx=0; indx < 4; indx++)
		for (i=0; i < BLKSIZE * p.l[indx]; i++) {
			if (pvec[p.pi[indx]][i % 32])
				*(obuf + k++) = OFFSET - 1 + (*(inbuf + j++) << 1);
			else
				*(obuf + k++) = OFFSET;
		}
	/* Depuncture remaining 24 bits using rate 8/16 */ 
	for (i=0; i < 24; i++)
		if (pvec[7][i % 32])
			*(obuf + k++) = OFFSET - 1 + (*(inbuf + j++) << 1);
		else
			*(obuf + k++) = OFFSET;
	*len = k;
	return 0;
}

int eep_depuncture(unsigned char *obuf, unsigned char *inbuf, struct audio_subch *s, int* len)
{
	int i, j, k, n, indx;
	struct eepprof p = eeptable[s->protlvl];

	/* Special case for bitrate == 8 with EEP 2-A */
	if ((s->bitrate == 8) && (s->protlvl == 1))
		p = eep2a8kbps;
	j = 0;
	k = 0;
	n = s->subchsz/p.sizemul;
	for (indx=0; indx < 2; indx++)
		for (i=0; i < BLKSIZE * (p.l[indx].mul * n + p.l[indx].offset); i++) {
			if (pvec[p.pi[indx]][i % 32])
				*(obuf + k++) = OFFSET - 1 + (*(inbuf + j++) << 1);
			else
				*(obuf + k++) = OFFSET;
		}
	/* Depuncture remaining 24 bits using rate 8/16 */ 
	for (i=0; i < 24; i++)
		if (pvec[7][i % 32])
			*(obuf + k++) = OFFSET - 1 + (*(inbuf + j++) << 1);
		else
			*(obuf + k++) = OFFSET;
	*len = k;
	return 0;
}
