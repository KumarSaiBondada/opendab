/*
    wfpk.c

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
#include "opendab.h"

int wfpk(double *magdata, int indx)
{
	double b, c, d, bmax = 0.0;
	int a, i, l;
	int pts = 0x800;
	int res = 0;

	a = indx - 0x1f8 + pts;
	b = 0x1f8 / 2;
	l = a + b - 1;

	for (i = 0; i < (2 * 0x1f8); i++) {
		c = *(magdata + ((pts - 0x1f8 / 2 + a) % 0x800));
		d = *(magdata + (l % 0x800));
		b = b + d - c;
		if (b > bmax) {
			bmax = b;
			res = a % 0x800;
		}
		l++;
		a++;
	}
  
	if (res > 0x400)
		res = res - 0x800;

	/* printf("wfpk: res=%d\n",res); */
	return (res);
}


