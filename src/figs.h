/*
    figs.h

    Copyright (C) 2007, 2008 David Crawley

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
** FIG-related bitfields, structures and tables
*/
/* Number of CUs in a symbol */
#define CUSPERSYM 48

/* Symbol at start of MSC */
#define MSCSTART 5

/* Number of symbols in a CIF */
#define SYMSPERCIF 18

/* Number of symbols in a CIF plus offset*/
#define OFFSETSYMS SYMSPERCIF+MSCSTART

/* Bytes in a CU */
#define BYTESPERCU 8

/* Bits in a CU */
#define BITSPERCU BYTESPERCU * 8

/* Bytes in a symbol */
#define BYTESPERSYM CUSPERSYM * 8

/* Bits in a symbol */
#define BITSPERSYM BYTESPERSYM * 8

/* sub-channel data */
struct audio_subch {
	int subchid;
	int startaddr;
	int subchsz;
	int bitrate;
	int eepprot; /* 1 if EEP, 0 if UEP */
	int protlvl;
	int uep_indx;
	int dabplus; /* 1 if DAB+, 0 otherwise */
};

struct data_subch_packet {
        int pktaddr;
        struct data_subch *subch;
};

struct data_subch {
	int subchid;
        int startaddr;
	int subchsz;
};

/* Used for sub-channel symbol extraction -
   can be part of a list if multiple sub-channels 
   are wanted
*/
struct symrange {
	int start[4];
	int end[4];
	int startcu;
	int endcu;
	int numsyms;
	struct symrange *next;
};

/* Number of services doesn't seem to have a fixed
   limit so this implementation uses a linked list
*/ 
struct service {
	char label[17];
	int sid;
        int scid;
	struct audio_subch *pa; /* Primary audio */
	struct audio_subch *sa; /* Secondary audio */
        struct data_subch_packet *dt;  /* Packet data */
	struct service *next;
	struct service *prev; 
};

/* Ensemble information */
struct ens_info {
	char label[17];
	unsigned short eid;
	int num_srvs;
	int num_schans;
	/* Pointer to list of services */
	struct service *srv;
	/* Array of subchannel info */
        union {
                struct audio_subch au;
                struct data_subch dt;
        } schan[64];
};

struct selsrv {
	int sid;
	struct audio_subch *au;
	struct data_subch_packet *dt;
	struct symrange sr; 
        struct cbuf *cbuf;
	unsigned int cur_frame;
        unsigned int cifcnt;
        FILE *dest;
};

/* FIG Header */
struct fig_hdr {
	unsigned flen    : 5;
	unsigned ftype   : 3;
};

struct fig_0 {
	unsigned extn    : 5;
	unsigned pd      : 1;
	unsigned oe      : 1;
	unsigned cn      : 1;
};

struct fig_1 {
	unsigned extn    : 3;
	unsigned oe      : 1;
	unsigned charset : 4;
};

/* Used by fig_0_0() */
struct ensinf {
	unsigned CIFCntL : 8;
	unsigned CIFCntH : 5;
	unsigned AlrmFlg : 1;
	unsigned ChgFlg  : 2;
};

/* Used by fig_0_1() Long Form */
struct subchorg_l {
	unsigned SubChSz   : 10;
	unsigned ProtLvl   : 2;
	unsigned Opt       : 3;
	unsigned LongForm  : 1;
	unsigned StartAddr : 10;
	unsigned SubChId   : 6;
};
/* Used by fig_0_1() Short Form (padded for alignment) */
struct subchorg_s {
	unsigned           : 8;
	unsigned TabIndx   : 6;
	unsigned TableSw   : 1;
	unsigned LongForm  : 1;
	unsigned StartAddr : 10;
	unsigned SubChId   : 6;
};

/* Used by fig_0_2 - streamed audio */
struct mscstau {
	unsigned CAFlag  : 1;
	unsigned Primary : 1;
	unsigned SubChId : 6;
	unsigned ASCTy   : 6;
	unsigned TMId    : 2;
};
/* Used by fig_0_2 - streamed data */
struct mscstdat {
	unsigned CAFlag  : 1;
	unsigned Primary : 1;
	unsigned SubChId : 6;
	unsigned DSCTy   : 6;
	unsigned TMId    : 2;
};
/* Used by fig_0_2 */
struct fidc {
	unsigned CAFlag  : 1;
	unsigned Primary : 1;
	unsigned FIDCId  : 6;
	unsigned DSCTy   : 6;
	unsigned TMId    : 2;
};
/* Used by fig_0_2 - packet data */
struct mscpktdat {
	unsigned CAFlag  : 1;
	unsigned Primary : 1;
	unsigned SCId    : 12;
	unsigned TMId    : 2;
};
/* Used by fig_0_2 */
struct lcn{
	unsigned NumSCmp : 4;
	unsigned CAId    : 3;
	unsigned Local   : 1;
};
/* Used by fig_0_2 */
struct ssid {
	unsigned SrvRef  : 12;
	unsigned CntryId : 4;
};
/* Used by fig_0_2 */
struct lsid {
	unsigned SrvRef  : 20;
	unsigned CntryId : 4;
	unsigned ExCC    : 8;
};

/* Used by fig_0_3 */
struct servcomp1 {
	unsigned SCCA    : 16;
	unsigned PktAddr : 10;
	unsigned SubChId : 6;
};
/* Used by fig_0_3 (padded for alignment) */
struct servcomp2 {
	unsigned         : 8; 
	unsigned DSCTy   : 6;
	unsigned Rfu     : 1;
	unsigned DGFlag  : 1;
	unsigned SCCAFlg : 1;
	unsigned Rfa     : 3;
	unsigned SCId    : 12;
};

/* Used by fig_0_10 */
struct utcl {
	unsigned UTCmsec : 10;
	unsigned UTCSec  : 6;
};
/* Used by fig_0_10 */
struct datim {
	unsigned UTCMin  : 6;
	unsigned UTCHour : 5;
	unsigned UTCFlg  : 1;
	unsigned ConfInd : 1;
	unsigned LSI     : 1;
	unsigned MJD     : 17;
	unsigned Rfu     : 1;
};

/* Used by fig_1_0 */  
struct el {
	short eid;
	char label[16];
	unsigned short chflag;
};

/* Used by fig_1_1 */ 
struct psl {
	short SId;
	char label[17];
	short CharFlgFld;
};

/* Used by fig_1_4 */
struct sclf {
	unsigned SCIds : 4;
	unsigned Rfa   : 3;
	unsigned PD    : 1;
};
/* Used by fig_1_4 */
struct scl {
	short CharFlgFld;
	char label[17];
	int SId;
	int SCIds;
	int Rfa;
	int PD;
}; 

/* Used by fig_1_5 */
struct dsl {
	int SId;
	char label[17];
	short CharFlgFld;
};
