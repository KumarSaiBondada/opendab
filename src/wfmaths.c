/*
  wfmaths.c

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

static void vec_reverse_real(fftw_complex *vec, int pts)
{
	/* The real part of the ifft has reverse order (apart from the
	   DC term) using fftw compared with the Intel SPL functions
	   used in the w!nd*ws software. */
        
	for (int i = 1; i < pts/2; i++) {
		double t = creal(*(vec+i));
		*(vec+i) = *(vec+i) - t + creal(*(vec+pts-i));
		*(vec+pts-i) = *(vec+pts-i) - creal(*(vec+pts-i)) + t;
	}
}

static void vec_reverse(fftw_complex *vec, int pts)
{
	/* The real part of the fft has reverse order using fftw
	   compared with the Intel SPL functions used in the w!nd*ws
	   software */

        for (int i = 1; i < pts/2; i++) {
                fftw_complex tc = *(vec+i);
                *(vec+i) = *(vec+pts-i);
                *(vec+pts-i) = tc;
        }
}

int fft_prs(fftw_complex *in, fftw_complex *out, int n)
{
	static fftw_plan fp;

	fp = fftw_plan_dft_1d(n, in, out, FFTW_FORWARD, FFTW_ESTIMATE);
	fftw_execute(fp);
        vec_reverse(out, n);
	return 0;
}

int ifft_prs(fftw_complex *in, fftw_complex *out, int n)
{
	static fftw_plan bp;

	bp = fftw_plan_dft_1d(n, in, out, FFTW_BACKWARD, FFTW_ESTIMATE);
	fftw_execute(bp);
        vec_reverse_real(out, n);
	return 0;
}

int mpy3(fftw_complex *srca, fftw_complex *srcb, fftw_complex *dst, int n)
{
	int k;

	for (k=0; k < n; k++) {
		*(dst + k) = (*(srca + k) * *(srcb + k));
		*(dst + k) = *(dst + k)/1024;
	} 
	return 0;

}

int mpy(fftw_complex *srca, fftw_complex *srcb, fftw_complex *dst, int n)
{
	int k;

	for (k=0; k < n; k++) {
		*(dst + k) = (*(srca + k) * *(srcb + k));
		*(dst + k) = *(dst + k)/32.0;
	} 
	return 0;

}

double maxext(double *in, int n, int *index)
{
	int i;
	double max;
	
	max = *in;
	*index = 0;

	for (i=1; i < n; i++)
		if (*(in+i) > max) {
			max = *(in+i);
			*index = i;
		}

	return(max);
}

int mag(fftw_complex *in, double *out, int n)
{
	int i;

	for (i=0; i < n; i++)
		*(out+i) = cabs(*(in+i))/n;

	return 0;
}

double mean(double *in, int n)
{
	int i;

	double out = 0.0L;

	for (i=0; i < n; i++)
		out += *(in+i);

	out = out/n;

	return(out);
}

