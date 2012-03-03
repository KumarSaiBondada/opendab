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

double raverage(struct raverage *r, double ir)
{
	int d, i;
	double t;

	if (fabs(r->prev_ir) > 350) {
		r->k = 0;
		r->j = 0;
	}

	r->sa[r->k++] = ir;

	if (r->k == 8) {
		r->k = 0;
		r->j = 1;
	}

	if (r->j != 0)
		d = 8;
	else
		d = r->k;

	t = 0.0;

	if (d > 0) {
		for  (i=0; i < d; i++)
			t = t + r->sa[i];
	}

	r->prev_ir = t / d;
	return (r->prev_ir);
}
