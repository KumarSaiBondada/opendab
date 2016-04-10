/*
    wfmsc.c

    Copyright (C) 2007 2008 David Crawley

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
**  Main Service Channel decoding etc.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "wfic/wfic.h"
#include "opendab.h"

#define DEBUG 0

/*
** Initialize circular buffer
*/
struct cbuf *init_cbuf(struct symrange *sr)
{
	int i;
        struct cbuf *cbuf;

        if ((cbuf = malloc(sizeof(struct cbuf))) == NULL)
                perror("init_cbuf malloc failed");

	for (i = 0; i < 16; i++) {
		cbuf->cb[i].next = &cbuf->cb[(i+1) % 16];
		if ((cbuf->cb[i].data = malloc(sizeof(unsigned char) * sr->numsyms * BITSPERSYM)) == NULL)
			perror("init_cbuf malloc failed");
		cbuf->cb[i].syms = 0;
		cbuf->cb[i].index = i;
	}

        cbuf->lfp = &cbuf->cb[0];
        cbuf->head = 0;
        cbuf->full = 0;
        cbuf->lframe = 0;

	return cbuf;
}

/*
 * Reset a circular buffer
 */
void reset_cbuf(struct cbuf *cbuf)
{
        int i;

        cbuf->lframe = 0;
        cbuf->lfp = &cbuf->cb[0];
        cbuf->head = 0;
        cbuf->full = 0;
        for (i=0; i < 16; i++)
                cbuf->cb[i].syms = 0;
}

/*
 * Write to a circular buffer
 */
int write_cbuf(struct cbuf *cbuf, unsigned char *symdata, int len, struct symrange *sr)
{
        int frame_complete = 0;

        if (cbuf->cb[cbuf->lframe].syms == sr->numsyms) {
                cbuf->head = (cbuf->head + 1) % 16;
                cbuf->lfp = cbuf->cb[cbuf->lframe].next; /* Next logical frame */
                cbuf->lframe = cbuf->lfp->index;
                cbuf->cb[cbuf->lframe].syms = 0;
        }
        if (cbuf->cb[cbuf->lframe].syms < sr->numsyms) {
                memcpy(cbuf->cb[cbuf->lframe].data + cbuf->cb[cbuf->lframe].syms * BITSPERSYM, symdata, len);
                cbuf->cb[cbuf->lframe].syms++;
                if (cbuf->cb[cbuf->lframe].syms == sr->numsyms)
                        frame_complete = 1;
        }

        if (cbuf->lframe == 15)
                cbuf->full = 1;

        return (cbuf->full && frame_complete);
}

/*
** Convert bytes to bits, reorder, frequency deinterleave
** and demap qpsk symbols using BDB's wfic functions
*/
int freq_deinterleave(unsigned char *obuf, unsigned char *cbuf)
{
	unsigned char bbuf[3072];
	unsigned char dbuf[3072];
	int len;

	byte_to_bit(NULL, 0, cbuf, 384, bbuf, &len);
	bitswap16(bbuf, dbuf, len);
	frequency_deinterleaver(dbuf, bbuf);
	qpsk_symbol_demapper(bbuf, obuf);

	return 0;
}

/*
** Time disinterleaving ETSI EN 300 401 V1.3.3 (2001-05), 12, P.161
** Multiplex reconfiguration has not yet been considered.
**
** Writes a single logical frame to obuf.
*/
int time_disinterleave(struct cbuf *cbuf, unsigned char *obuf, int subchsz, struct symrange *sr)
{
	int i, cif;
	const int map[] = {0,8,4,12,2,10,6,14,1,9,5,13,3,11,7,15};

	for (i=0; i < (subchsz * BITSPERCU); i++) {
		cif = map[i % 16];
		*(obuf+i) = *(cbuf->cb[(cbuf->head + cif) % 16].data + BITSPERCU * sr->startcu + i);
	}
	return 0;
}

/*
** Finish decoding the frames from the circular buffer.
*/
int msc_decode(struct selsrv *srv)
{
/* Constraint length */
#define N 4
/* Number of symbols per data bit */
#define K 7

	static int init = 1;
	static unsigned char *lf, *dpbuf, *obuf, *sfbuf;
	unsigned int metric;
	int bits, len, obytes;

        struct audio_subch *au = srv->au;
        struct data_subch_packet *dt = srv->dt;
        struct symrange *sr = &srv->sr;

        int subchsz;
        if (au != NULL)
                subchsz = au->subchsz;
        else if (dt != NULL)
                subchsz = dt->subch->subchsz;
        else
                return 0;

	if (init) {
		init = 0;
		if ((lf = malloc(subchsz * BITSPERCU * sizeof(unsigned char))) == NULL) {
			perror("msc_decode lf");
			exit(EXIT_FAILURE);
		}
		if ((dpbuf = malloc(4 * subchsz * BITSPERCU * sizeof(unsigned char))) == NULL) {
			perror("msc_decode dpbuf");
			exit(EXIT_FAILURE);
		}
		if ((obuf = malloc(4 * subchsz * BITSPERCU * sizeof(unsigned char))) == NULL) {
			perror("msc_decode obuf");
			exit(EXIT_FAILURE);
		}
		if (au != NULL && au->dabplus) {
			if ((sfbuf = malloc(5 * 4 * subchsz * BITSPERCU * sizeof(unsigned char))) == NULL) {
				perror("msc_decode sfbuf");
				exit(EXIT_FAILURE);
			}
			wfinitrs(); /* Initialize the Reed-Solomon decoder */
		}

	}

	time_disinterleave(srv->cbuf, lf, subchsz, sr);

        if (au != NULL) {
                if (au->eepprot)
                        eep_depuncture(dpbuf, lf, au, &len);
                else
                        uep_depuncture(dpbuf, lf, au, &len);
        }
        if (dt != NULL) {
                eep_depuncture_data(dpbuf, lf, dt->subch, &len);
        }

	/* BDB's wfic functions: */
	bits = len/N - (K - 1);
	k_viterbi(&metric, obuf, dpbuf, bits, mettab, 0, 0);
	scramble(NULL, obuf, dpbuf, bits);
	bit_to_byte(NULL, 1, dpbuf, bits, obuf, &obytes);

        if (au != NULL) {
                if (au->dabplus) {
#ifdef DABPLUS
                        wfdabplusdec(sfbuf, obuf, obytes, au->bitrate, srv->dest);
#else
                        fprintf(stderr,"Built without DAB+ support - enable in Makefile and rebuild\n");
#endif
                }
                else {
                        wfmp2(obuf, obytes, au->bitrate, srv->dest, srv->pad);
                }

                wfpad(srv->pad, obuf, obytes);
        }
        else {
                wfdata(srv->data, obuf, obytes, srv->dest);
        }

	return 0;
}

/*
** Copy symbols corresponding to the given subchannel,
** after frequency deinterleaving, to a circular buffer.
** Complete processing once buffer is full.
*/
int msc_assemble(unsigned char *symbuf, struct selsrv *srv)
{
	unsigned char sym, frame;
	unsigned char fbuf[BITSPERSYM];
	int j, buffer_full = 0;

	sym = *(symbuf+2);
	frame = *(symbuf+3);

	if (sym == srv->sr.start[0]) {
		srv->cur_frame = frame;
		freq_deinterleave(fbuf, symbuf+12);
		buffer_full = write_cbuf(srv->cbuf, fbuf, BITSPERSYM, &srv->sr);
	} else {
		for (j=1; j < 4; j++)
			if (sym == srv->sr.start[j]) {
                                //fprintf(stderr, "sym: %d frame: %d\n", sym, frame);

				if (frame != srv->cur_frame) {
					reset_cbuf(srv->cbuf);
				} else {
					freq_deinterleave(fbuf, symbuf+12);
					buffer_full = write_cbuf(srv->cbuf, fbuf, BITSPERSYM, &srv->sr);
				}
			}
		for (j=0; j < 4; j++)
			if ((sym > srv->sr.start[j]) && (sym <= srv->sr.end[j])) {
                                //fprintf(stderr, "sym: %d frame: %d\n", sym, frame);

				if (frame != srv->cur_frame) {
                                        reset_cbuf(srv->cbuf);
				} else {
					freq_deinterleave(fbuf, symbuf+12);
					buffer_full = write_cbuf(srv->cbuf, fbuf, BITSPERSYM, &srv->sr);
					if (sym == srv->sr.end[j])
						srv->cifcnt++;
				}
			}
	}
	if (buffer_full) {
                fprintf(stderr, "buffer full\n");
		msc_decode(srv);
        }

	return 0;
}
