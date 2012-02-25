/*
    wfref.c

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
#define _XOPEN_SOURCE 1
#include "opendab.h"

extern fftw_complex *prs_cread(const char*, int);
extern void cpx_dump(char*, fftw_complex*, int);

int *cos_table;
fftw_complex *prs1, *prs2;

int wfref(int indx, int pts, fftw_complex* outp, fftw_complex* inp)
{
	if (indx == 0) {
		memcpy(outp, inp, pts*sizeof(fftw_complex));
	} else {
		if (indx > 0) {
			memcpy(outp, inp + pts - indx, indx*sizeof(fftw_complex));
			memcpy(outp + indx, inp, (pts - indx)*sizeof(fftw_complex));
		} else {
			if (indx < 0) {
				indx = -indx;
				memcpy(outp + pts - indx , inp, indx*sizeof(fftw_complex));
				memcpy(outp, inp + indx, (pts - indx)*sizeof(fftw_complex));
			}
		}
	}
	return 0;
}

/*
** Load PRS data, build cosine table
*/
int wfrefinit(int fd)
{
	int i;

	prs1 = prs_cread("prs1.gplot",0x1000);
	prs2 = prs_cread("prs2.gplot",0x1000);

	cos_table = (int*)malloc(0x800 * sizeof(int));

	for (i=0; i < 0x800; i++) 
		*(cos_table + i) = 32767 * cos(i * 2 * M_PI/2048);

	return 0;
}
