/*
    opendab.h

    Copyright (C) 2007 David Crawley

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

#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200112L
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <math.h>
#include <complex.h>
#include <fftw3.h>

#include "wfsl11r.h"
#include "figs.h"
#include "pad.h"

/* TODO: eliminate these declarations - should be unnecessary */
#define USB_TYPE_VENDOR                 (0x02 << 5)
typedef unsigned char __u8;
typedef unsigned short __u16;

struct usb_ctrlrequest {
        __u8 bRequestType;
        __u8 bRequest;
        __u16 wValue;
        __u16 wIndex;
        __u16 wLength;
	} __attribute__ ((packed));
/* TODO: eliminate the above declarations - should be unnecessary */

#include "../driver/wavefinder.h"

#define TRUE -1;
#define FALSE 0;

/* USB Device identifiers for WaveFinder */
#define VENDOR 0x9cd
#define PRODUCT 0x2001

/* WaveFinder bmRequest values */

#define bmRequest1 1
#define bmRequest2 2
#define SLMEM 3
#define WFTUNE 4
#define WFTIMING 5

/* Size of read buffer */
#define PIPESIZE 16768

struct cbuf *init_cbuf(struct symrange *sr);

struct blist {
	unsigned char* data;
	int syms;
	int index;
	struct blist *next;
};

struct cbuf {
        struct blist cb[16];
	struct blist *lfp; 
	int lframe;
        int full;
        int head;
};

struct sync_state {
        int count;
        int locked;
        int lock_count;
        long last_cv_msg;
        long last_afc_msg;
        double afc_offset;
        unsigned char seen_flags;
        unsigned char *prsbuf;
        struct raverage *ravg;
        struct sync_refs *refs;
};

struct sync_refs {
        fftw_complex *prs1;
        fftw_complex *prs2;
        int *cos_table;
};

struct raverage {
        int j;
        int k;
        double prev_ir;
        double sa[8];
};

struct data_state {
        int addr;
        int dgtype;
};

int wfmp2(unsigned char *buf, int len, int bitrate, FILE *dest, struct pad_state *pad);

long wf_time(void);

int wfinitrs();
int wfdabplusdec(unsigned char *sfbuf, unsigned char *ibuf, int ibytes, int bitrate, FILE *dest);

struct sync_state *wfsyncinit();
int prs_assemble(int fd, unsigned char *rdbuf, struct sync_state *sync);

int msc_assemble(unsigned char *symbuf, struct selsrv *srv);
int fic_assemble(unsigned char* rdbuf, unsigned char* ficsyms, unsigned char* rawfibs, FILE *ofp);
int startsym_audio(struct symrange *r, struct audio_subch *s);
int startsym_data(struct symrange *r, struct data_subch *s);
int ficinit(struct ens_info *e);
int wfsymsel(unsigned char *sel, struct symrange *r);
int unpickfig(unsigned char* fig, int figlen);

int user_select_service(struct ens_info* e, struct selsrv *sel_srv);
int label_select_service(struct ens_info* e, struct selsrv *sel_srv, char *label);
int disp_ensemble(struct ens_info* e);
struct service* find_service(struct ens_info *e, int sid);
struct service* find_service_by_scid(struct ens_info *e, int scid);
int add_audio_service(struct ens_info *e, struct mscstau *ac, int sid);
int add_data_service(struct ens_info *e, struct mscpktdat *dt, int sid);
int add_audio_subchannel(struct ens_info *e, struct audio_subch *s);
int add_data_subchannel(struct ens_info *e, struct data_subch *s);
int labelled(struct ens_info *e);

int uep_depuncture(unsigned char *obuf, unsigned char *inbuf, struct audio_subch *s, int* len);
int eep_depuncture(unsigned char *obuf, unsigned char *inbuf, struct audio_subch *s, int* len);
int eep_depuncture_data(unsigned char *obuf, unsigned char *inbuf, struct data_subch *s, int* len);

int crc16check(unsigned char* buf, int len);
int crccheck(unsigned char* buf, int len);
int firecrccheck(unsigned char *buf);

fftw_complex* prs_cread(const char* prsfile, int n);
int prs_scale(unsigned char *prsdate, fftw_complex *idata);

int fft_prs(fftw_complex *in, fftw_complex *out, int n);
int ifft_prs(fftw_complex *in, fftw_complex *out, int n);
int mpy3(fftw_complex *srca, fftw_complex *srcb, fftw_complex *dst, int n);
int mpy(fftw_complex *srca, fftw_complex *srcb, fftw_complex *dst, int n);
int mag(fftw_complex *in, double *out, int n);
double mean(double *in, int n);
double maxext(double *in, int n, int *index);
double wfimp(struct sync_state *sync, double irtime, fftw_complex *mdata);
double raverage(struct raverage *r, double ir);
int wfpk(double *magdata, int indx);

void cplx_dump(char *fname, fftw_complex *vect, int pts);
void cpx_dump(char *fname, fftw_complex *vect, int pts);
void dbl_dump(char *fname, double *vect, int pts);
void char_dump(char* fname, unsigned char* vect, int pts);

void *wfrefinit(struct sync_state *sync);
int wfref(int indx, int pts, fftw_complex* outp, fftw_complex* inp);

void wf_sleep(int usec);

int wf_init(int fd, double freq);
void wf_tune(int fd, double freq);
int wfcatch(int fd);
int wfgetnum(int max);
int wf_close(int fd);
int wf_timing(int fd, int msgnum);
int wf_read(int fd, unsigned char *rdbuf, int *len);
int wf_afc(int fd, double *offset, double afcv);
int wf_mem_write(int fd, unsigned short addr, unsigned short val);
int wf_sendmem(int fd, int value, int index, unsigned char *bytes, int size);
int wf_r1_msg(int fd, unsigned char *bytes);
int wf_r2_msg(int fd, unsigned char *bytes);
int wf_leds(int fd, int red, int blue, int green);
int wf_leds_off(int fd);
int wf_timing_msg(int fd, unsigned char* bytes);

void wfpad(struct pad_state *pad, unsigned char *buf, int bytes);
struct pad_state *init_pad(void);

int wfdata(struct data_state *d, unsigned char *buf, int pktlen, FILE *dest);
struct data_state *init_data(int pktaddr);
