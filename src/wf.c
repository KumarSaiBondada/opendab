/*
  wf.c

  Copyright (C) 2005-2008  David Crawley

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
** Main program interoperating with the Psion WaveFinder 
*/
#include "opendab.h"

#define MAXFIBS 700

extern struct ens_info einf;
extern int fibcnt;

/* Select all symbols by default */
static unsigned char selstr[] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff};

int fdw;
unsigned char *fsyms, *rfibs, *rdbuf;
FILE *of = NULL, *ffp = NULL;

int main (int argc, char **argv)
{
	struct selsrv sel_srv;
        char outfile[80] = "";
	char devname[80] = "/dev/wavefinder0";
	char ficfile[80] = "fic.dat";
        const char usage[] = " is part of OpenDAB. Distributed under the GPL V.3\nUsage: wf [-f] [-d outfile] [-w devname][freq]\n-d dumps ensemble to outfile caution - huge!\n-w uses Wavefinder devname\n-f generates FIC file 'fic.dat' (ignored if '-d' specified)\nfreq defaults to 225.648MHz (BBC National DAB)";	
	int fd, k, nargs, sym;
        int l = 0, gen_fic = 0, gen_dump = 0;
	int slock = 0, enslistvisible = 0;
	int selected = 0;
        struct sync_state *sync;

	/* double freq = 218.640;*/ /* DRg */
	/* double freq = 222.064;*/ /* Digital 1 */
	/* double freq = 223.936;*/
	double freq = 225.648; /* BBC */
	/* double freq = 227.360;*/
	
        nargs = argc;
        while (nargs-- > 1) {
                if (*argv[argc-nargs] == '-') {
                        switch (*(argv[argc-nargs]+1)) {
                        case 'd':
                                /* Set output filename */
                                if (strcmp(argv[argc-nargs], "-d") == 0) {
					if (--nargs) {
						strcpy(outfile, argv[argc-(nargs)]);
						gen_dump = 1;
					} else {
						fprintf(stderr,"Please supply a filename for -d\n");
						fprintf(stderr,"%s %s\n", argv[0], usage);
						exit(EXIT_FAILURE);
					}
					 break;	
				}
                                break;
			case 'w':
				/* Set device name */
				if (strcmp(argv[argc-nargs], "-w") == 0) {
					if (--nargs) {
						strcpy(devname, argv[argc-(nargs)]);
					} else {
						fprintf(stderr,"Please supply a device name for -w\n");
						fprintf(stderr,"%s %s\n", argv[0], usage);
						exit(EXIT_FAILURE);
					}
					 break;	
				}
                                break;
                        case 'f':
                                if (strcmp(argv[argc-nargs], "-f") == 0)
                                        gen_fic = 1;
                                break;
                        case 'h':
                                printf("%s %s\n", argv[0], usage);
                                exit(EXIT_SUCCESS);
                                break;
			};
		} else
			sscanf(argv[argc-nargs],"%le",&freq);
	}
	/* Open WaveFinder */	
	fd = open(devname,O_RDWR);
	if (fd == -1) {
		perror(devname);
		exit(EXIT_FAILURE);
	}

	wfcatch(fd);   /* Install handler to catch SIGTERM */

	if (gen_dump) {
		if ((of = fopen(outfile,"w")) == NULL) {
			perror("Output file open failed");
			wf_close(fd);
		}
	} else if (gen_fic)
		if ((ffp = fopen(ficfile,"w")) == NULL) {
			perror("FIC output file open failed");
			wf_close(fd);
		}

	
	/* Initialize synchronization system - PRS data, storage etc */
	if ((sync = wfsyncinit()) == NULL) {
                perror("wfsyncinit failed");
                wf_close(fd);
        }

	/* Initialize read buffer storage */
	if ((rdbuf = calloc(PIPESIZE, sizeof(unsigned char))) == NULL) {
		fprintf(stderr,"main: calloc failed for rdbuf");
		wf_close(fd);
	}

	/* Allocate storage for symbols 2, 3 and 4 which comprise the FIC */
	if ((fsyms = calloc(384 * 3, sizeof(unsigned char))) == NULL) {
		fprintf(stderr,"main: calloc failed for fsyms");
		wf_close(fd);
	}

	/* Allocate storage for decoded, but "unparsed" FIC
	   - allow for 16 FIBs, each minus its CRC
	*/
	if ((rfibs = calloc(360*16, sizeof(unsigned char))) == NULL) {
		fprintf(stderr,"main: calloc failed for rfibs");
		wf_close(fd);
	}

	if (!gen_dump) {
		sel_srv.au = (struct audio_subch *)NULL;
		sel_srv.dt = (struct data_subch_packet *)NULL;
		ficinit(&einf);
	}

	/* Initalize Wavefinder */
	fprintf(stderr,"Initialization ");
	wf_init(fd, freq);
	fprintf(stderr,"complete.\n");

	fprintf(stderr,"Sync ");
	for (;;) {
		wf_read(fd, rdbuf, &l);
		for (k=0; k < l; k+=524) {
                        prs_assemble(fd, (rdbuf + k), sync);

			if (sync->locked && !slock) {
				slock = 1;
				fprintf(stderr,"locked.\n");
			}
			if (gen_dump && (*(rdbuf+9+k) == 0x01))
				fwrite(rdbuf+k, sizeof(unsigned char), 524, of);
			else
				/* Wait for sync to lock or things might get slowed down too much */
				if (sync->locked) {
					if ((fibcnt < MAXFIBS) && !labelled(&einf)) {
						sym = *(rdbuf+2+k);
						if ((sym == 2)||(sym == 3)||(sym == 4))
							fic_assemble(rdbuf+k, fsyms, rfibs, ffp);
					} else {
						if (!enslistvisible) {
							disp_ensemble(&einf);
							enslistvisible = 1;
						}
						if (sel_srv.au == NULL && sel_srv.dt == NULL) {
                                                        user_select_service(&einf, &sel_srv);
                                                }
						else {
							if (!selected && sel_srv.au != NULL && (sel_srv.au->subchid < 64)) {
								startsym_audio(&sel_srv.sr, sel_srv.au);
								wfsymsel(selstr, &sel_srv.sr);
                                                                sel_srv.cbuf = init_cbuf(&sel_srv.sr);
								selected = 1;
								sync->count = 6; // sync
								fprintf(stderr,"Type ctrl-c to quit\n");
							}
							if (!selected && sel_srv.dt != NULL && (sel_srv.dt->subch->subchid < 64)) {
								startsym_data(&sel_srv.sr, sel_srv.dt->subch);
								wfsymsel(selstr, &sel_srv.sr);
                                                                sel_srv.cbuf = init_cbuf(&sel_srv.sr);
								selected = 1;
								sync->count = 6;
								fprintf(stderr,"Type ctrl-c to quit\n");
							}
							if ((sync->count == 0) && (*(rdbuf+2+k) > 4))
                                                                msc_assemble(rdbuf+k, &sel_srv);
						}
					}
				}
		}
	}
	exit(EXIT_SUCCESS);	
}

