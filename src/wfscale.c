/*
  wfscale.c

  Copyright (C) 2005-2008 David Crawley

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

/*
** Rescales and removes offset from the PRS read from the WaveFinder
*/
int prs_scale(unsigned char* prsdata, fftw_complex* idata)
{
	int k;
	unsigned char *dptr;

	dptr = prsdata;

	for (k = 0; k < 0x800; k++)  
		/* *(idata + k) = 0 + 0.732421875*(*(dptr + k) - 128)*I; */
		*(idata + k) = 0 + (*(dptr + k) - 128)*I;

	return 0;
}
