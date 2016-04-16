#include "opendab.h"

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
        wf = wf_open(devname);
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

        wf->service->au = (struct audio_subch *)NULL;
        wf->service->dt = (struct data_subch_packet *)NULL;

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
	unsigned char sym, frame;
        static unsigned char cur_frame, last_sym;

        prs_assemble(wf, buf, wf->sync);
        if (wf->sync->locked && !wf->sync->slock) {
                wf->sync->slock = 1;
                fprintf(stderr,"locked.\n");
        }

	sym = *(buf+2);
	frame = *(buf+3);

        if (sym == last_sym)
                return 0;

        last_sym = sym;

        if (cur_frame != frame) {
                cur_frame = frame;
                //fprintf(stderr, "\n%d: ", frame);
        }

        //if (sym >= 5)
        //        fprintf(stderr, "%d ", sym);

        fwrite(buf, sizeof(unsigned char), 524, stdout);

        return 0;
}
