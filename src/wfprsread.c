/*
  wfprsread.c

  Copyright (C) 2006, 2007, 2008 David Crawley

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

fftw_complex* prs_cread(const char* prsfile, int n)
{
	FILE *ifp;
	fftw_complex *prsbuf, *prsptr;
	double r, i;

	if ((ifp = fopen(prsfile, "r")) == NULL) {
		fprintf(stderr,"Couldn't open complex prs file");
		exit(EXIT_FAILURE);
	}

	if ((prsbuf = fftw_malloc(sizeof(fftw_complex) * n)) == NULL) {
		fprintf(stderr,"fftw_malloc failed");
		exit(EXIT_FAILURE);
	}

	prsptr = prsbuf;

	while (!feof(ifp)) {
		fscanf(ifp,"%lf %lf",&r,&i);
		if (!feof(ifp)) {
			*(prsptr++) = r + i*I;
		}
	}
	return(prsbuf);
}
