/*
  wfcmd.c

  Copyright (C) 2005 David Crawley

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
** Handle terminal IO
** Necessary in order to keep the sync loop operating
** and close the wavefinder correctly on exit.
*/
#include "opendab.h"

extern FILE *of, *ffp;
static struct wavefinder *wfw;

int wfgetnum(int max)
{
	fd_set rfds;
	struct timeval tv;
	int retval;
	int v, c;
        char lbuf[80];

	FD_ZERO(&rfds);
	FD_SET(0, &rfds);

	tv.tv_sec = 0;
	tv.tv_usec = 0;

	retval = select(1, &rfds, NULL, NULL, &tv);

	if (retval == -1) {
		perror("select()");
                return(-1);
	}
	else if (retval) {
		if ((c = read(STDIN_FILENO, lbuf, 80 * sizeof(unsigned char))) < 0) {
                        perror("read()");
                        return(-1);
                }
		if (sscanf(lbuf,"%d",&v) != 1)
			v = -1;
                if ((v < 0) || (v > max))
                        v = -1;
                        
	} else {
		v = -1;
        }

	return (v);
}

/*
** Close files, shut down WaveFinder and exit
*/
void wfexit(int signum)
{
	if (of != NULL)
		fclose(of);
	if (ffp != NULL)
		fclose(ffp);

	wf_close(wfw);
	fprintf(stderr,"Done.\n");
	exit(EXIT_SUCCESS);
}

/*
** Catch SIGINT (ctrl-c) to exit cleanly
*/ 
int wfcatch(struct wavefinder *wf)
{
	struct sigaction a;

        wfw = (struct wavefinder *)malloc(sizeof(struct wavefinder));
        memcpy(wfw, wf, sizeof(struct wavefinder));
	a.sa_handler = wfexit;
	sigemptyset(&(a.sa_mask));
	a.sa_flags = 0;
  
	sigaction(SIGINT, &a, NULL);

	return 0;
}

