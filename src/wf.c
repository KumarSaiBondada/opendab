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

unsigned char *fsyms, *rfibs;
FILE *of = NULL;
FILE *ffp = NULL;

int main (int argc, char **argv)
{
	char devname[80] = "/dev/wavefinder0";
        const char usage[] = " is part of OpenDAB. Distributed under the GPL V.3\nUsage: wf [-f] [-d outfile] [-w devname][freq]\n-d dumps ensemble to outfile caution - huge!\n-w uses Wavefinder devname\n-f generates FIC file 'fic.dat' (ignored if '-d' specified)\nfreq defaults to 225.648MHz (BBC National DAB)";
	int nargs;
        struct wavefinder *wf;

	/* double freq = 218.640;*/ /* DRg */
	/* double freq = 222.064;*/ /* Digital 1 */
	/* double freq = 223.936;*/
	double freq = 225.648; /* BBC */
	/* double freq = 227.360;*/

        nargs = argc;
        while (nargs-- > 1) {
                if (*argv[argc-nargs] == '-') {
                        switch (*(argv[argc-nargs]+1)) {
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
                        case 'h':
                                printf("%s %s\n", argv[0], usage);
                                exit(EXIT_SUCCESS);
                                break;
			};
		} else
			sscanf(argv[argc-nargs],"%le",&freq);
	}
	/* Open WaveFinder */
        wf = wf_open(devname, wf_process_packet);
	if (wf == NULL) {
		perror("wavefinder");
		exit(EXIT_FAILURE);
	}

	/* Initialize synchronization system - PRS data, storage etc */
	if ((wf->sync = wfsyncinit()) == NULL) {
                perror("wfsyncinit failed");
                wf_close(wf);
                exit(EXIT_FAILURE);
        }

        if ((wf->service = calloc(1, sizeof(struct selsrv))) == NULL) {
                perror("wf->service calloc failed");
                wf_close(wf);
                exit(EXIT_FAILURE);
        }

        wf->init = 1;
        wf->selected = 0;
        wf->enslistvisible = 0;

	wfcatch(wf);   /* Install handler to catch SIGTERM */

	/* Allocate storage for symbols 2, 3 and 4 which comprise the FIC */
	if ((fsyms = calloc(384 * 3, sizeof(unsigned char))) == NULL) {
		fprintf(stderr,"main: calloc failed for fsyms");
		wf_close(wf);
	}

	/* Allocate storage for decoded, but "unparsed" FIC
	   - allow for 16 FIBs, each minus its CRC
	*/
	if ((rfibs = calloc(360*16, sizeof(unsigned char))) == NULL) {
		fprintf(stderr,"main: calloc failed for rfibs");
		wf_close(wf);
	}

        wf->service->au = (struct audio_subch *)NULL;
        wf->service->dt = (struct data_subch_packet *)NULL;
        ficinit(&einf);

	/* Initalize Wavefinder */
	fprintf(stderr,"Initialization ");
	wf_init(wf, freq);
	fprintf(stderr,"complete.\n");

        wf->init = 0;

	fprintf(stderr,"Sync ");
        wf_read(wf);
	exit(EXIT_SUCCESS);
}

int wf_process_packet(struct wavefinder *wf, unsigned char *buf)
{
        int sym = *(buf+2);

        prs_assemble(wf, buf, wf->sync);

        if (wf->sync->locked && !wf->sync->slock) {
               wf->sync->slock = 1;
               fprintf(stderr,"locked.\n");
        }
        /* Wait for sync to lock or things might get slowed down too much */
        if (wf->sync->locked) {
                if ((fibcnt < MAXFIBS) && !labelled(&einf)) {
                        if ((sym == 2)||(sym == 3)||(sym == 4))
                                fic_assemble(buf, fsyms, rfibs, ffp);
                } else {
                        if (!wf->enslistvisible) {
                                disp_ensemble(&einf);
                                wf->enslistvisible = 1;
                        }
                        if (wf->service->au == NULL && wf->service->dt == NULL) {
                                user_select_service(&einf, wf->service);
                        }
                        else {
                                if (!wf->selected && wf->service->au != NULL && (wf->service->au->subchid < 64)) {
                                        startsym_audio(&wf->service->sr, wf->service->au);
                                        wfsymsel(selstr, &wf->service->sr);
                                        wf->service->cbuf = init_cbuf(&wf->service->sr);
                                        wf->service->pad = init_pad();
                                        wf->selected = 1;
                                        wf->sync->count = 6; // sync
                                        fprintf(stderr,"Type ctrl-c to quit\n");
                                }
                                if (!wf->selected && wf->service->dt != NULL && (wf->service->dt->subch->subchid < 64)) {
                                        startsym_data(&wf->service->sr, wf->service->dt->subch);
                                        wfsymsel(selstr, &wf->service->sr);
                                        wf->service->cbuf = init_cbuf(&wf->service->sr);
                                        wf->service->data = init_data(wf->service->dt->pktaddr);
                                        wf->selected = 1;
                                        wf->sync->count = 6;
                                        fprintf(stderr,"Type ctrl-c to quit\n");
                                }
                                if ((wf->sync->count == 0) && (*(buf+2) > 4)) {
                                        msc_assemble(buf, wf->service);
                                }
                        }
                }
        }
        return 0;
}
