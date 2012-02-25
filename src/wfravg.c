/*
  wfravg.c

  Copyright (C) 2005 David Crawley

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

double raverage(double ir)
{
	static double prev_ir = 0, sa[] = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
	static int j = 0, k = 0;
	int d, i;
	double t;

	/* printf("prev_ir = %e ir = %e k = %d\n",prev_ir,ir,k); */
	if (fabs(prev_ir) > 350) {
		k = 0;
		j = 0;
	}

	sa[k++] = ir;

	if (k == 8) {
		k = 0;
		j = 1;
	}

	if (j != 0)
		d = 8;
	else
		d = k;

	t = 0.0;

	if (d > 0) {
		for  (i=0; i < d; i++)
			t = t + sa[i];
	}

	prev_ir = t / d;
	/* printf("sa=%e %e %e %e %e %e %e %e\n",sa[0],sa[1],sa[2],sa[3],sa[4],sa[5],sa[6],sa[7]);
	   printf("Post raverage prev_ir = %e\n",prev_ir); */
	return (prev_ir);
}
