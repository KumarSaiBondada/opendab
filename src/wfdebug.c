/*
  wfdebug.c

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
/*
** Debug printing functions
*/

#include "opendab.h"

void stream_dump(fftw_complex *vect, int pts)
{
	int j;

}

/* Dump vector of fttw_complex to file suitable for gnuplot */

void cplx_dump(char* fname, fftw_complex* vect, int pts)
{
	FILE *dfp;
	int j;

	if ((dfp = fopen(fname, "w")) == NULL) {
		fprintf(stderr,"cplx_dump: Couldn't open output file\n");
		exit(EXIT_FAILURE);
	}
  
	/* fputs("# Real part\n",dfp); */
	/* for (j=0; j < pts; j++) */
	/* 	fprintf(dfp,"%e\n",creal(*(vect+j))); */

	/* fputs("\n\n# Imaginary part\n",dfp); */
	/* for (j=0; j < pts; j++) */
	/* 	fprintf(dfp,"%e\n",cimag(*(vect+j))); */

	for (j=0; j < pts; j++)
                fprintf(dfp, "%e %e\n", creal(*(vect+j)), cimag(*(vect+j)));

	fclose(dfp);
}

/* Dump vector of fttw_complex to file suitable for gnuplot
   Alternative format */

void cpx_dump(char* fname, fftw_complex* vect, int pts)
{
	FILE *dfp;
	int j;

	if ((dfp = fopen(fname, "w")) == NULL) {
		fprintf(stderr,"cplx_dump: Couldn't open output file\n");
		exit(EXIT_FAILURE);
	}
  
	for (j=0; j < pts; j++)
		fprintf(dfp,"%e  %e\n",creal(*(vect+j)),cimag(*(vect+j)));

	fclose(dfp);
}

/* Dump vector of doubles to file suitable for gnuplot */

void dbl_dump(char* fname, double* vect, int pts)
{
	FILE *dfp;
	int j;

	if ((dfp = fopen(fname, "w")) == NULL) {
		fprintf(stderr,"dbl_dump: Couldn't open output file\n");
		exit(EXIT_FAILURE);
	}
  
	fputs("# Real vector\n",dfp);
	for (j=0; j < pts; j++)
		fprintf(dfp,"%e\n",*(vect+j));

	fclose(dfp);
}

/* Dump vector of char to file suitable for gnuplot */

void char_dump(char* fname, unsigned char* vect, int pts)
{
	FILE *dfp;
	int j;

	if ((dfp = fopen(fname, "w")) == NULL) {
		fprintf(stderr,"char_dump: Couldn't open output file\n");
		exit(EXIT_FAILURE);
	}
  
	fputs("# char vector\n",dfp);
	for (j=0; j < pts; j++)
		fprintf(dfp,"%hhu\n",*(vect+j));

	fclose(dfp);
}
