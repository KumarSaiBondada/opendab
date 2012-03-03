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

int fdw;
unsigned char *fsyms, *rfibs, *rdbuf;
FILE *of = NULL, *ffp = NULL;

/* no-op */
int wfgetnum(int max)
{
        return 0;
}

int main(int argc, char **argv)
{
	FILE *ifp = NULL;
	char infile[80] = "raw.strm";
	const char usage0[] = " is part of OpenDAB. Distributed under the GPL V.3";
	const char usage1[] = "Usage: wfx [-f] [-o outfile] [infile]";
	const char usage2[] = "infile defaults to \"raw.strm\", outfile to \"out.mp2\" -f generates FIC file fic.dat";
	int nargs;
	int fd = 0, k;
        int l = 0;
        struct sync_state *sync;

	nargs = argc;
	while (nargs-- > 1) {
		if (*argv[argc-nargs] == '-') {
			switch (*(argv[argc-nargs]+1)) {
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

	/* Initialize synchronization system - PRS data, storage etc */
	sync = wfsyncinit();

	/* Initialize read buffer storage */
	if ((rdbuf = calloc(PIPESIZE, sizeof(unsigned char))) == NULL) {
		fprintf(stderr,"main: calloc failed for rdbuf");
		exit(EXIT_FAILURE);
	}

	fprintf(stderr,"Sync ");

	while (!feof(ifp)) {
		l = fread(rdbuf, 16768, 1, ifp);
                l *= 16768;
                
		for (k=0; k < l; k+=524) {
                        prs_assemble(fd, (rdbuf + k), sync);
		}
	}
	exit(EXIT_SUCCESS);	
}

