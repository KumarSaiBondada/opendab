/*
  wfmp2.c

  Copyright (C) 2008 David Crawley

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
#include <stdio.h>
#include <string.h>
#include "wfbyteops.h"
#include "opendab.h"

/*
** Check that the audio frame pointed to by 'buf'
** is valid - if not then it is silently ignored
*/
int wfmp2(unsigned char *buf, int len, int bitrate)
{
/* These header bits are constant for DAB */
#define HMASK 0xfff70e03
/* ..and have these values.. */
#define HXOR 0xfff40400

	struct mp2header {
		unsigned emphasis       : 2;
		unsigned orig           : 1;
		unsigned copyright      : 1;
		unsigned mode_extension : 2;
		unsigned mode           : 2;
		unsigned private_bit    : 1;
		unsigned padding_bit    : 1;
		unsigned sampling_freq  : 2;
		unsigned bit_rate_index : 4;
		unsigned protection_bit : 1;
		unsigned layer          : 2;
		unsigned id             : 1;
		unsigned syncword       : 12;
	} mp2h;

	/* Tables 20 and 21 ETSI EN 300 401 V1.3.3 (2001-05), 7.2.1.3, P.69-70
	   Entries of -1 in these tables correspond to forbidden indices */
	const int brtab[] = {-1,32,48,56,64,80,96,112,128,160,192,224,256,320,384,-1}; 
	const int lbrtab[] = {-1,8,16,24,32,40,48,56,64,80,96,112,128,144,160,-1}; 

	int i;
	static int header_valid;
	static int header_expected = 1;

	if (header_expected) {
		header_valid = 1;
		i = ipack(buf);
		memcpy(&mp2h, &i, sizeof(struct mp2header));
		if (((i & HMASK) ^ HXOR) != 0)
			header_valid = 0;
		else if (mp2h.id == 0) {
			header_expected = 0;
			if (lbrtab[mp2h.bit_rate_index] != bitrate) {
				/* fprintf(stderr,"Low bitrate conflict FIC:%d MP2 header:%d\n",bitrate,lbrtab[mp2h.bit_rate_index]); */
				header_valid = 0;
			}
		} else if (brtab[mp2h.bit_rate_index] != bitrate) {
			/* fprintf(stderr,"Bitrate conflict FIC:%d MP2 header:%d\n",bitrate,brtab[mp2h.bit_rate_index]); */
			header_valid = 0;
		}
	} else
		header_expected = 1;

	if (header_valid)
		fwrite(buf, sizeof(char), len, stdout);

	return 0;
}
