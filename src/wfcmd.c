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

int wfgetnum(int max)
{
	int i;

	scanf("%d",&i);

	while ((i < 0) || (i > (max))) {
		fprintf(stderr,"Please select a service (0 to %d): ", max);
		scanf("%d",&i);
	}
	return i;
}

