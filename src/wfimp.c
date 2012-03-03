/*
    wfimp.c

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
#include "opendab.h"

double wfimp(struct sync_state *sync, double irtime, fftw_complex *mdata)
{
	double ir, re_prs, im_prs, ri, rj;
	int jr;
	int a = 0x800, d, a1c, m;
	int i, cosa, cosd, p, q, r, s;
	double j = 0.0, k = 0.0;

	ir = irtime * 4096.0;
	jr = (int)ir & 0x7fffff;

	m = a;

	for (i=0; i < 0x800; i++) {
		a = m;
		m = m + jr;
		a = a >> 12;
		d = a & 0x7ff;
		a -= 512;
		a &= 0x7ff;
		/* Too slow! */
		/* cosa = (1 << 15)*cos((double)a * 2*M_PI/2048.0L); */
		/* cosd = (1 << 15)*cos((double)d * 2*M_PI/2048.0L); */
		cosa = *(sync->refs->cos_table + a);
		cosd = *(sync->refs->cos_table + d);
		re_prs = creal(*(mdata + i));
		im_prs = cimag(*(mdata + i));  
		p = im_prs * cosd;
		q = im_prs * cosa;
		r = re_prs * cosa;
		s = re_prs * cosd;
		s = s - q;
		r = r + p;
		k += s;
		j += r;
	}
	ri = sqrt((k*k) + (j*j));
	a1c = 0x800 * 32767;

	rj = 1/(double)a1c * ri;
	/* printf("rj = %e\n",rj); */
	return(rj);
}


