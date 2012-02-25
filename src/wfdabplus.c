/*
    wfdabplus.c

    Copyright (C) 2008 David Crawley

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
#include "wfbyteops.h"
#include "opendab.h"

#ifdef DABPLUS
#include <fec.h>
#endif

#ifdef DABPLUS
static void *rs;
#endif

struct stream_parms {
	int rfa;
	int dac_rate;
	int sbr_flag;
	int ps_flag;
	int aac_channel_mode;
	int mpeg_surround_config;
};


/*
** Initialize the Reed-Solomon decoder
** Parameters as ETSI TS 102 563 V1.1.1 (2007-02) sect. 6.1, p.13
** 
*/
int wfinitrs()
{
#ifdef DABPLUS
	/* symsize=8, gfpoly=0x11d, fcr=0, prim=1, nroots=10, pad=135 */
	if ((rs = init_rs_char(8, 0x11d, 0, 1, 10, 135)) == (void*)NULL)
		perror("wfinitrs failed");
#endif
	return 0;
}

/*
** Write an Audio Data Transport Stream (ADTS) header as defined by
** ISO/IEC 14496-3
*/
int wfadts(int framelen, struct stream_parms *sp)
{
	struct adts_fixed_header {
		unsigned                     : 4;
		unsigned home                : 1;
		unsigned orig                : 1;
		unsigned channel_config      : 3;
		unsigned private_bit         : 1;
		unsigned sampling_freq_index : 4;
		unsigned profile             : 2;
		unsigned protection_absent   : 1;
		unsigned layer               : 2;
		unsigned id                  : 1;
		unsigned syncword            : 12;
	} fh;
	struct adts_variable_header {
		unsigned                            : 4;
		unsigned num_raw_data_blks_in_frame : 2;
		unsigned adts_buffer_fullness       : 11;
		unsigned frame_length               : 13;
		unsigned copyright_id_start         : 1;
		unsigned copyright_id_bit           : 1;
	} vh;

	unsigned char header[7];

	/* 32k 16k 48k 24k */
	const unsigned short samptab[] = {0x5, 0x8, 0x3, 0x6}; 

	fh.syncword = 0xfff;
	fh.id = 0;
	fh.layer = 0;
	fh.protection_absent = 1;
	fh.profile = 0;
	fh.sampling_freq_index = samptab[sp->dac_rate << 1 | sp->sbr_flag];

	fh.private_bit = 0;
	switch (sp->mpeg_surround_config) {
	case 0:
		if (sp->sbr_flag && !sp->aac_channel_mode && sp->ps_flag)
			fh.channel_config = 2; /* Parametric stereo */
		else
			fh.channel_config = 1 << sp->aac_channel_mode ;
		break;
	case 1:
		fh.channel_config = 6;
		break;
	default:
		fprintf(stderr,"Unrecognized mpeg_surround_config ignored\n");
		if (sp->sbr_flag && !sp->aac_channel_mode && sp->ps_flag)
			fh.channel_config = 2; /* Parametric stereo */
		else
			fh.channel_config = 1 << sp->aac_channel_mode ;
		break;
	}

	fh.orig = 0;
	fh.home = 0;
	vh.copyright_id_bit = 0;
	vh.copyright_id_start = 0;
	vh.frame_length = framelen + 7;  /* Includes header length */
	vh.adts_buffer_fullness = 1999;
	vh.num_raw_data_blks_in_frame = 0;

	header[0] = fh.syncword >> 4;
	header[1] = (fh.syncword & 0xf) << 4;
	header[1] |= fh.id << 3;
	header[1] |= fh.layer << 1;
	header[1] |= fh.protection_absent;
        header[2] = fh.profile << 6;
	header[2] |= fh.sampling_freq_index << 2;
	header[2] |= fh.private_bit << 1;
	header[2] |= (fh.channel_config & 0x4);
	header[3] = (fh.channel_config & 0x3) << 6;
	header[3] |= fh.orig << 5;
	header[3] |= fh.home << 4; 
	header[3] |= vh.copyright_id_bit << 3;
	header[3] |= vh.copyright_id_start << 2;
	header[3] |= (vh.frame_length >> 11) & 0x3;
	header[4] = (vh.frame_length >> 3) & 0xff;
	header[5] = (vh.frame_length & 0x7) << 5;
	header[5] |= vh.adts_buffer_fullness >> 6;
	header[6] = (vh.adts_buffer_fullness & 0x3f) << 2;
	header[6] |= vh.num_raw_data_blks_in_frame;

	fwrite(header, sizeof(header), 1, stdout);
	return 0;
}





/* 
** Decode logical frames for DAB+
** ETSI TS 102 563 V1.1.1 (2007-02) Annex C describes possible
** strategies for synchronization.
**
** For now, I'm going to assume the Reed-Solomon decoding could be slow.
**
** Accumulate 5 frames in sfbuf - the first frame
** must begin with a valid audio superframe header.
*/
int wfdabplusdec(unsigned char* sfbuf, unsigned char* ibuf, int ibytes, int bitrate, FILE *dest)
{
	static int frame = 0;
	const int austab[4] = {4, 2, 6, 3};
	int s = bitrate/8;
#ifdef DABPLUS
	int errs;
#endif
	int i, j;
	unsigned char cbuf[120];
	struct stream_parms sp;
	int num_aus;
	unsigned int au_start[6];
	int au_size[6];
	int audio_super_frame_size = ibytes * 5 - s * 10; /* Excludes error prot bytes */

	memset(au_start, 0, sizeof(au_start));
	memset(au_size, 0, sizeof(au_size));

	if ((frame == 0) && firecrccheck(ibuf)) {
		frame++;
		memcpy(sfbuf, ibuf, ibytes);
	}
	else if (frame > 0) 
		memcpy(sfbuf + ibytes * frame++, ibuf, ibytes);
	
	/* Process if superframe complete */
	if (frame > 4) {
		frame = 0;
		for (i = 0; i < s; i++) {
			/* Dis-interleaving is necessary prior to RS decoding */
			for (j = 0; j < 120; j++)
				cbuf[j] = *(sfbuf + s * j + i);
#ifdef DABPLUS
			errs = decode_rs_char(rs, cbuf, (int*)NULL, 0);
#endif
			/* fprintf(stderr,"errs = %d\n",errs);*/
			/* Write checked/corrected data back to sfbuf */
			for (j = 0; j < 110; j++)
				*(sfbuf + s * j + i) = cbuf[j];
		}
		sp.rfa = (*(sfbuf+2) & 0x80) && 1;
		sp.dac_rate = (*(sfbuf+2) & 0x40) && 1;
		sp.sbr_flag = (*(sfbuf+2) & 0x20) && 1;
		sp.aac_channel_mode = (*(sfbuf+2) & 0x10) && 1; 
		sp.ps_flag = (*(sfbuf+2) & 0x8) && 1;
		sp.mpeg_surround_config = *(sfbuf+2) & 0x7;
		num_aus = austab[sp.dac_rate << 1 | sp.sbr_flag];
		switch (num_aus) {
		case 2: 
			au_start[0] = 5;
			au_start[1] = (*(sfbuf + 3) << 4) + ((*(sfbuf + 4)) >> 4); 
			au_size[0] = au_start[1] - au_start[0];
			au_size[1] = audio_super_frame_size - au_start[1];
			break;
		case 3: 
			au_start[0] = 6;
			au_start[1] = (*(sfbuf + 3) << 4) + ((*(sfbuf + 4)) >> 4);
			au_start[2] = ((*(sfbuf + 4) & 0x0f) << 8) + *(sfbuf + 5);
			au_size[0] = au_start[1] - au_start[0];
			au_size[1] = au_start[2] - au_start[1];
			au_size[2] = audio_super_frame_size - au_start[2];
			break;
		case 4: 
			au_start[0] = 8;
			au_start[1] = (*(sfbuf + 3) << 4) + (*(sfbuf + 4) >> 4);
			au_start[2] = ((*(sfbuf + 4) & 0x0f) << 8) + *(sfbuf + 5);
			au_start[3] = (*(sfbuf + 6) << 4) + (*(sfbuf + 7) >> 4);
			au_size[0] = au_start[1] - au_start[0];
			au_size[1] = au_start[2] - au_start[1];
			au_size[2] = au_start[3] - au_start[2];
			au_size[3] = audio_super_frame_size - au_start[3];
			break;
		case 6: 
			au_start[0] = 11;
			au_start[1] = (*(sfbuf + 3) << 4) + (*(sfbuf + 4) >> 4);
			au_start[2] = ((*(sfbuf + 4) & 0x0f) << 8) + *(sfbuf + 5);
			au_start[3] = (*(sfbuf + 6) << 4) + (*(sfbuf + 7) >> 4);
			au_start[4] = ((*(sfbuf + 7) & 0x0f) << 8) + *(sfbuf + 8);
			au_start[5] = (*(sfbuf + 9) << 4) + (*(sfbuf + 10) >> 4);
			au_size[0] = au_start[1] - au_start[0];
			au_size[1] = au_start[2] - au_start[1];
			au_size[2] = au_start[3] - au_start[2];
			au_size[3] = au_start[4] - au_start[3];
			au_size[4] = au_start[5] - au_start[4];
			au_size[5] = audio_super_frame_size - au_start[5];
			break;
		default: fprintf(stderr,"num_aus = %d is invalid\n",num_aus);

			break;
		}
		for (i = 0; i < num_aus; i++) {
			/* Invert CRC bits */
			*(sfbuf + au_start[i] + au_size[i] - 2) ^= 0xff;
			*(sfbuf + au_start[i] + au_size[i] - 1) ^= 0xff;
			/* AUs with bad CRC are silently ignored */
			if (crccheck(sfbuf + au_start[i], au_size[i])) {
				wfadts(au_size[i]-2, &sp);
				fwrite(sfbuf + au_start[i], sizeof(unsigned char), au_size[i] - 2, dest);
			}
		}
	}
	return 0;
}
