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

static fftw_complex *idata, *rdata, *mdata, *prslocal, *iprslocal, *cdata;
static double *magdata;

/* Use this before changing the symbol selection */
static unsigned char chgstr[] = {0x00, 0xf0, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11};

static void wf_sync_cvmsg(struct wavefinder *wf, double c)
{
        unsigned short cv;
        int i = c * -8192000.0;

        if (i != 0) {

                i = i + 0x7f;
                if (i > 0) {
                        cv = i;
                        if (cv > 0xff)
                                cv = 0xff;
                }
                else {
                        cv = 0;
                }

                cv = 0x1000 | cv;
                wf_mem_write(wf, OUTREG0, cv);
        }
}

static void wf_sync_imsg(struct wavefinder *wf, double ir)
{
	unsigned char imsg[] = {0x7f, 0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
				0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00,
				0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff,
				0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0f, 0x00};

	short w1 = ((int)(ir * 81.66400146484375)) & 0xffff;
	short w2 = ((int)(ir * 1.024)) & 0xffff;

        unsigned char *symstr;

        if (wf->sync->count > 0) {
                symstr = chgstr;
                wf->sync->count--;
        } else {
                symstr = wf->selstr;
        }

	memcpy(imsg + 2, symstr, 10 * sizeof(unsigned char));
	imsg[24] = w1 & 0xff;
	imsg[25] = (w1 >> 8) & 0xff;
	imsg[26] = w2 & 0xff;
	imsg[27] = (w2 >> 8) & 0xff;

	wf_timing_msg(wf, imsg);
}

static double calc_c(struct sync_state *sync, fftw_complex *idata, int pts)
{
        int lcnt, indx;
        int indx_n = 0, indxv = 0;
	double maxv = 0;
	double c = 4.8828125e-7;

	ifft_prs(idata, rdata, pts);

	if (sync->locked) {
		wfref(0x0, 0x800, prslocal, sync->refs->prs1);
		lcnt = 1;
	} else {
		wfref(0x0c, 0x800, prslocal, sync->refs->prs1);
		lcnt = 25;
	}

	/* Copy 0x18 complex points from start of data and append to the end */
	for (int i = 0; i < 0x18; i++)
		*(prslocal + pts + i) = *(prslocal + i);

	for (int j = 0; j < lcnt; j++) {
		mpy3(rdata, prslocal + j, cdata, pts);
		fft_prs(cdata, mdata, pts);
		mag(mdata, magdata, pts);

		double max = maxext(magdata, pts, &indx);
		double vmean = mean(magdata, pts);

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

	if (sync->locked)
		c = c * indx_n;
	else
		c = c * indxv;

	wfref(-indxv, pts, iprslocal, sync->refs->prs2);

        return c;
}

static double calc_ir(struct sync_state *sync, fftw_complex *idata, int pts)
{
        int indx;
        double ir, v, stf;

	mpy(idata, iprslocal, mdata, pts);
	fft_prs(mdata, rdata, pts);
	mag(rdata, magdata, pts);

	double max = maxext(magdata, pts, &indx);
	double vmean = mean(magdata, pts);

	if ((14.0 * vmean) > max)
		max = 0.0;

	ir = indx;

	if (ir >= 0x400)
		ir -= 0x800;

	stf = 0.666666666;

	do {
		stf = stf/2;
		v = ir - stf;

		double vi = wfimp(sync, v, mdata);
		if (vi > max) {
			max = vi;
			ir = v;
		}
		v = ir + stf;
		double vs = wfimp(sync, v, mdata);
		if (vs > max) {
			max = vs;
			ir = v;
		}
	} while ((1000*stf) > 2.5e-2);

	ir *= 1000.0;

        return ir;
}

int wf_sync(struct wavefinder *wf)
{
	int pts = 0x800;
        struct sync_state *sync = wf->sync;

	prs_scale(sync->prsbuf, idata);

        /* this also sets up iprslocal */
        double c = calc_c(sync, idata, pts);

        double ir = calc_ir(sync, idata, pts);

	if ((fabs(c) < (2.4609375e-4/2)) && (fabs(ir) < 350)) {
		if (sync->lock_count == 0) {
			sync->locked = 1;
		} else {
			sync->lock_count--;
			sync->locked = 0;
		}
	} else {
                /* fprintf(stderr, "NOT OK\n"); */
		sync->lock_count = 3;
		sync->locked = 0;
        }

        //fprintf(stderr, "c: %0.10f ir: %0.10f sync_locked: %d lock_count: %d count: %d\n", c, ir, sync->locked, sync->lock_count, sync->count);

	/* Must be at least 60ms between these messages */
	long ems = wf_time();

        //fprintf(stderr, "ems: %ld\n", ems);

	long dt = ems - sync->last_cv_msg;
	if (dt > 60L) {
                //fprintf(stderr, "cv c: %0.10f\n", c);
                wf_sync_cvmsg(wf, c);
                sync->last_cv_msg = ems;
	}

	ir = raverage(sync->ravg, ir);

	/* Must be at least 250ms between AFC messages */
	dt = ems - sync->last_afc_msg;
        if (dt > 250L) {
                //fprintf(stderr, "afc offset: %.10f ir: %.10f\n", sync->afc_offset, ir);
                wf_afc(wf, &sync->afc_offset, ir);
                sync->last_afc_msg = ems;
        }

        /* Send this message every time */
        wf_sync_imsg(wf, ir);

	return 0;
}

/*
** Assemble the appropriate four packets which form
** the Phase Reference Symbol and then call
** wf_sync() to do the synchronization
*/
int prs_assemble(struct wavefinder *wf, unsigned char *rdbuf)
{
	int blk;
        struct sync_state *sync = wf->sync;
        
        if (*(rdbuf+9) != 0x02) {
                return(0);
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
			wf_sync(wf);
		}
	}
	return(0);
}

struct sync_state *wfsyncinit(void)
{
        int i;
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

	if ((sync->ravg = malloc(sizeof(struct raverage))) == NULL) {
		fprintf(stderr,"wfsync: malloc failed for ravg");
                return(NULL);
	}

        sync->slock = 0;
        sync->count = 0;
        sync->locked = 0;
        sync->lock_count = 3;
        sync->seen_flags = 0;
        sync->last_cv_msg = 0;
        sync->last_afc_msg = 0;
	sync->afc_offset = 3.25e-1;

        for (i = 0; i < 8; i++) {
                sync->ravg->sa[i] = 0.0;
        }
        sync->ravg->prev_ir = 0.0;
        sync->ravg->j = 0;
        sync->ravg->k = 0;

	if (wfrefinit(sync) == NULL) {
		fprintf(stderr,"wfsync: wfrefinit failed");
                return(NULL);
	}

        return sync;
}
