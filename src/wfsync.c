/*
    wfsync.c

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
/*
** Use the received Phase Reference Signal from
** the WaveFinder to generate the necessary timing
** values sent back in the control messages with
** bRequest = 5
*/
#include "opendab.h"

extern fftw_complex *prs1, *prs2;

static double vmean;
static fftw_complex *idata, *rdata, *mdata, *prslocal, *iprslocal;
static double *magdata;

/* Select all symbols by default */
static unsigned char selstr[] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
/* Use this before changing the symbol selection */
static unsigned char chgstr[] = {0x00, 0xf0, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11};

fftw_complex *cdata;

int wf_sync(int fd, unsigned char *symstr, struct sync_state *sync)
{
	static int cnt = 0;
	static int lckcnt = 3;
	static long lms = 0L;
	long ems, dt;
	int i, j, indxv = 0, indx, indx_n = 0, lcnt;
	double t,u,v, vi, vs, max, maxv, stf, ir, c;
	short w1, w2;
	unsigned short cv;
	int pts = 0x800;
	unsigned char imsg[] = {0x7f, 0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
				0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00,
				0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff,
				0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0f, 0x00};

	fftw_complex tc;

	struct timespec tp;

	prs_scale(sync->prsbuf, idata);
	ifft_prs(idata, rdata, pts);

	/* The real part of the ifft has reverse order (apart from
	   the DC term) using fftw compared with the Intel SPL functions
	   used in the w!nd*ws software. */
	for (i=1; i < pts/2; i++) {
		t = creal(*(rdata+i));
		*(rdata+i) = *(rdata+i) - t + creal(*(rdata+pts-i));
		*(rdata+pts-i) = *(rdata+pts-i) - creal(*(rdata+pts-i)) + t;
	}

	if (sync->locked) {
		wfref(0x0, 0x800, prslocal, prs1);
		lcnt = 1;
	} else {
		wfref(0x0c, 0x800, prslocal, prs1);
		lcnt = 25;
	}
	/* Copy 0x18 complex points from start of data and append to the end */
	for (i=0; i < 0x18; i++)
		*(prslocal + 0x800 + i) = *(prslocal + i);

	maxv = 0;

	for (j=0; j < lcnt; j++) {
		mpy3(rdata, prslocal + j, cdata, pts);
		fft_prs(cdata, mdata, pts);

		/* The real part of the fft has reverse order using fftw compared
		   with the Intel SPL functions used in the w!nd*ws software */
		for (i=1; i < pts/2; i++) {
			tc = *(mdata+i);
			*(mdata+i) = *(mdata+pts-i);
			*(mdata+pts-i) = tc;
		}
		mag(mdata, magdata, pts);
		max = maxext(magdata, pts, &indx);
		vmean = mean(magdata, pts);

		if ((vmean * 12) > max)
			max = 0;

		if (sync->locked) {
			indx_n = wfpk(magdata, indx);
			indx_n = indx_n/15;

			if (indx_n > 12)
				indx_n = 80;
			else
				if (indx_n < -12)
					indx_n = -80;
			
			indx_n = -indx_n;
		}

		if (max > maxv) {
			maxv = max;
			indxv = indx;
		}
	}

	if (indxv < 0x400)
		indxv = -indxv;
	else
		indxv = 0x800 - indxv;

	c = 4.8828125e-7;

	if (sync->locked)
		c = c * indx_n;
	else
		c = c * indxv;

	wfref(-indxv, 0x800, iprslocal, prs2);
	mpy(idata, iprslocal, mdata, pts);
	fft_prs(mdata, rdata, pts);

	/* The real part of the fft has reverse order using fftw compared
	   with the Intel SPL functions used in the w!nd*ws software */
	for (i=1; i < pts/2; i++) {
		tc = *(rdata+i);
		*(rdata+i) = *(rdata+pts-i);
		*(rdata+pts-i) = tc;
	}

	mag(rdata, magdata, pts);
	max = maxext(magdata, pts, &indx);
	vmean = mean(magdata, pts);

	if ((14.0 * vmean) > max)
		max = 0.0;

	ir = indx;

	if (ir >= 0x400)
		ir -= 0x800;

	stf = 0.666666666;

	do {
		stf = stf/2;
		v = ir - stf;
    
		vi = wfimp(v, mdata);
		if (vi > max) {
			max = vi;
			ir = v;
		}
		v = ir + stf;
		vs = wfimp(v, mdata);
		if (vs > max) {
			max = vs;
			ir = v;
		}
	} while ((1000*stf) > 2.5e-2); 

	ir = ir * 1000.0;

	if ((fabs(c) < (2.4609375e-4/2)) && (fabs(ir) < 350)) {
		if (lckcnt == 0) {
			sync->locked = 1;
		} else {
			lckcnt--;
			sync->locked = 0;
		}
	} else {
		lckcnt = 3;
		sync->locked = 0;
	}

        fprintf(stderr, "c: %0.10f sync_locked: %d lckcnt: %d\n", c, sync->locked, lckcnt);

	i = c * -8192000.0;

        wf_time(&tp);
	tp.tv_sec = tp.tv_sec % 1000000;
	ems = tp.tv_sec * 1000 + tp.tv_nsec/1000000;
	dt = ems - lms;
	/* Allow at least 60ms between these messages */
	if (dt > 60L) {
		if (i != 0) {
			i = i + 0x7f;
			if (i > 0) {
				cv = i;
				if (cv > 0xff)
					cv = 0xff;
			} else cv = 0;

			cv = 0x1000 | cv;
			wf_mem_write(fd, OUTREG0, cv);
		}
	}
	lms = ems;

	ir = raverage(ir);
	wf_afc(fd, ir);
	
	t = ir * 81.66400146484375;
	w1 = (int)t & 0xffff;
	u = ir * 1.024;
	w2 = (int)u & 0xffff;

	memcpy(imsg + 2, symstr, 10 * sizeof(unsigned char));
	cnt++;
	imsg[24] = w1 & 0xff;
	imsg[25] = (w1 >> 8) & 0xff;
	imsg[26] = w2 & 0xff;  
	imsg[27] = (w2 >> 8) & 0xff;
	wf_timing_msg(fd, imsg);

	return 0;
}

/*
** Assemble the appropriate four packets which form
** the Phase Reference Symbol and then call
** wf_sync() to do the synchronization
*/ 
int prs_assemble(int fd, unsigned char *rdbuf, struct sync_state *sync)
{
	int blk;
        unsigned char *symstr;
        
        if (*(rdbuf+9) != 0x02) {
                return(0);
        }
        
        if (sync->count > 0) {
                symstr = chgstr;
                sync->count--;
        } else {
                symstr = selstr;
        }

	blk = *(rdbuf+7); /* Block number: 0-3 */

	if (blk == 0x00) {
		sync->seen_flags = 1;
		memcpy(sync->prsbuf, rdbuf+12, 512);
	}  else {
		sync->seen_flags |= 1 << blk;
		/* Copy block data, excluding header */ 
		memcpy(sync->prsbuf+(blk*512), rdbuf+12, 512);
    
		if (sync->seen_flags == 15) {
			sync->seen_flags = 0;
			wf_sync(fd, symstr, sync);
		}
	}
	return(0);
}

struct sync_state *wfsyncinit(void)
{
        struct sync_state *sync;
        
	/* alloc storage for impulse response calculations */
	if ((idata = calloc(0x800, sizeof(fftw_complex))) == NULL) {
		fprintf(stderr,"wfsync: calloc failed for idata");
		return(NULL);
	}

	if ((cdata = calloc(0x800, sizeof(fftw_complex))) == NULL) {
		fprintf(stderr,"wfsync: calloc failed for cdata");
		return(NULL);
	}

	if ((rdata = calloc(0x800, sizeof(fftw_complex))) == NULL) {
		fprintf(stderr,"wfsync: calloc failed for rdata");
		return(NULL);
	}

	if ((mdata = calloc(0x800, sizeof(fftw_complex))) == NULL) {
		fprintf(stderr,"wfsync: calloc failed for mdata");
		return(NULL);
	}

	if ((prslocal = calloc(0x820, sizeof(fftw_complex))) == NULL) {
		fprintf(stderr,"wfsync: calloc failed for prslocal");
		return(NULL);
	}

	if ((iprslocal = calloc(0x820, sizeof(fftw_complex))) == NULL) {
		fprintf(stderr,"wfsync: calloc failed for iprslocal");
		return(NULL);
	}

	if ((magdata = calloc(0x800, sizeof(double))) == NULL) {
		fprintf(stderr,"wfsync: calloc failed for magdata");
		return(NULL);
	}

        /* initialise a struct sync_state */
        if ((sync = malloc(sizeof(struct sync_state))) == NULL) {
		fprintf(stderr,"wfsync: malloc failed for sync");
                return(NULL);
	}
        
	if ((sync->prsbuf = calloc(0x800, sizeof(unsigned char))) == NULL) {
		fprintf(stderr,"wfsync: calloc failed for prsbuf");
                return(NULL);
	}

        sync->count = 0;
        sync->locked = 0;
        sync->seen_flags = 0;

	wfrefinit();
	return sync;
}
