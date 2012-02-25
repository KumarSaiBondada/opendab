/*
    wfread.c

    Copyright (C) 2005,2006,2007,2008 David Crawley

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
** Read incoming data from the WaveFinder.
**
** There is one isochronous stream which contains
** (at least) two types of data:
**
** (i) Phase Reference Symbol (PRS) used for synchronization
** (ii) QPSK symbols
**
** Data arrives in 524-byte blocks which have a 12-byte
** header
**
** 4 PRS blocks are needed to make a 2048 byte PRS (Mode I)
**
**   0c 62 01 05 36 20 04 03 00 02 00 00 58 c0 66 6a
**   ^^ ^^ 		  ^^    ^^       ^^ 
** Constant	      Block no.	PRS      Data starts here
**
** Other symbols forming the FIC and MSC look like this:
**   0c 62 07 0e f8 20 01 00 80 01 00 00 2f 3e d9 ab
**   ^^ ^^ ^^ ^^                ^^       ^^
**     |   |   Frame no.        QPSK     Data starts here
**     |   Symbol no.
**     |   (0-76)
** Constant
**
*/
#include "opendab.h"

int wf_read(int fd, unsigned char *rdbuf, int* len)
{
	int l = 0;
	
	while (l == 0)
		l = read(fd,rdbuf,PIPESIZE);

	if (l < 0)
		perror("wf_read");
	else
		*len = l;

	return 0;
}

