/*
  wfleds.c

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
** Control the WaveFinder LEDs, the most important
** aspect of which is the ability to turn them off.
*/
#include "opendab.h"

int wf_mem_write(int, unsigned short, unsigned short);

/*
** Set red, green and blue LED intensities on the given WaveFinder
** A negative colour component causes that component to be unchanged 
*/
int wf_leds(int fd, int red, int blue, int green)
{
	if (!(red < 0)) {
		wf_mem_write(fd, PWMCH2STOP, red);
	}

	if (!(green < 0)) {
		wf_mem_write(fd, PWMCH1STOP, green);
	}

	if (!(blue < 0)) {
		wf_mem_write(fd, PWMCH3STOP, blue);
	}

	return 0;
  
}

/*
** Turn LEDs off convenience function
*/
int wf_leds_off(int fd)
{
	return(wf_leds(fd,0x3ff, 0x3ff, 0x3ff));
}
