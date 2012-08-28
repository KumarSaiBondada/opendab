/*
    wfafc.c

    Copyright (C) 2007-2008 David Crawley

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
** Send AFC value to control the VCXO DAC in the WaveFinder
*/
#include "opendab.h"

int wf_afc(struct wavefinder *wf, double *offset, double afcv)
{
	double a;
	int i;
	short afc_val;

	if ((afcv > 75) || (afcv < -75)) {
		if ((afcv > 350) || (afcv < -350)) {
			a = afcv * -2.2e-5;
			a = a + *offset;
			*offset = a;
		} else {
			a = 1.0/4096.0;
			if (afcv > 0.0)
				a = -a;
			a = *offset + a;
			*offset = a;
		}

		i = (int)(a * 65535);
	
		if (i > 0xffff)
			i = 0xffff;
		
		afc_val = i & 0xfffc;
		wf_mem_write(wf, DACVALUE, afc_val);
	}
        
        return 0;
}

