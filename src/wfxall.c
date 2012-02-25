/*
    wfx.c

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
** Extract audio data from a stream of raw symbol data 
** from a file generated using the "-d" option of wf.
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "opendab.h"

extern struct ens_info einf;

static unsigned char pktbuf[524];
static unsigned char fsyms[384 * 3];
static unsigned char rfibs[360];

/* Skip this many symbols at the start of the file - likely to be bad */
#define FSKIP 800

/* Replaces wfgetnum used in wf */ 
int wfgetnum(unsigned char *buf)
{
	int i;

	scanf("%d",&i);

	while ((i < 0) || (i > (einf.num_srvs-1))) {
		fprintf(stderr,"Please select a service (0 to %d): ",einf.num_srvs-1);
		scanf("%d",&i);
	}
	return i;
}

int main(int argc, char **argv)
{
	FILE *ifp, *ofp = NULL;
	char infile[80] = "raw.strm";
	const char usage0[] = " is part of OpenDAB. Distributed under the GPL V.3";
	const char usage1[] = "Usage: wfx [-f] [-o outfile] [infile]";
	const char usage2[] = "infile defaults to \"raw.strm\", outfile to \"out.mp2\" -f generates FIC file fic.dat";
	int nargs, cnt, gen_fic = 0, f = 0;

        int i;
        FILE *outs[10];
        char *fname;
	struct selsrv sel_srv[10];
        struct service *p;

	nargs = argc;
	while (nargs-- > 1) {
		if (*argv[argc-nargs] == '-') {
			switch (*(argv[argc-nargs]+1)) {
			case 'f':
				if (strcmp(argv[argc-nargs], "-f") == 0)
					gen_fic = 1;
				break;
			case 'h':
				fprintf(stderr,"%s %s\n", argv[0], usage0);
				puts(usage1);
				puts(usage2);
				exit(EXIT_SUCCESS);
				break;
			default:
				fprintf(stderr,"Unknown option -%c\n",*(argv[argc-nargs]+1));
				fprintf(stderr,"%s %s\n", argv[0], usage0);
				puts(usage1);
				puts(usage2);
				exit(EXIT_SUCCESS);
				break;	
			}
		} else
			strcpy(infile,argv[argc-nargs]);
	}

	if ((ifp = fopen(infile,"r")) == NULL) {
		fprintf(stderr,"Couldn't open input file %s\n",infile);
		exit(EXIT_FAILURE);
	}

	if (gen_fic && ((ofp = fopen("fic.dat","w")) == NULL)) {
		fprintf(stderr,"Failed to open fic.dat for writing\n");
		exit(EXIT_FAILURE);
	}

	ficinit(&einf);

	while (!feof(ifp) && !labelled(&einf)) {
		cnt = fread(pktbuf, 524, 1, ifp);
		if ((cnt == 1) && (*pktbuf == 0x0c) && (*(pktbuf+1) == 0x62)) 
			if ((*(pktbuf+2) == 2)||(*(pktbuf+2) == 3)||(*(pktbuf+2) == 4))
				fic_assemble(pktbuf, fsyms, rfibs, ofp);
	}
        
	rewind(ifp);
	disp_ensemble(&einf);

        i = 0;
        for (p = einf.srv; p != NULL; p = p->next) {
                (void) asprintf(&fname, "service_%x.mp2", p->sid);
                outs[i] = fopen(fname, "w");
                free(fname);

                sel_srv[i].sch = p->pa; /* XXX primary hardcoded */
                sel_srv[i].sid = p->sid;
                sel_srv[i].cur_frame = 0;
                sel_srv[i].cifcnt = 0;
                sel_srv[i].dest = outs[i];
                startsym(&sel_srv[i].sr, sel_srv[i].sch);
                sel_srv[i].cbuf = init_cbuf(&sel_srv[i].sr);

                i++;
        }
        
	while (!feof(ifp)) {
		cnt = fread(pktbuf, 524, 1, ifp);
		if ((f++ > FSKIP) && (cnt == 1) && (*pktbuf == 0x0c) && (*(pktbuf+1) == 0x62)) {
                        i = 0;
                        for (p = einf.srv; p != NULL; p = p->next) {
                                if ((sel_srv[i].sch != NULL) && (*(pktbuf+2) > 4)) {
                                        msc_assemble(pktbuf, &sel_srv[i]);
                                }
                                i++;
                        }
                }
	}
	fclose(ifp);
	if (gen_fic)
		fclose(ofp);

        i = 0;
        for (p = einf.srv; p != NULL; p = p->next) {
                fclose(outs[i]);
                i++;
        }

	exit(EXIT_SUCCESS);
}
