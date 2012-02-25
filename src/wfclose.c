/*
  wfclose.c

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
** Turn LEDs off, send the necessary control word, close device 
*/

#include "opendab.h"

int wf_mem_write(int, unsigned short, unsigned short);
int wf_leds_off(int);

int wf_close(int fd)
{
	wf_leds_off(fd);
	wf_mem_write(fd, OUTREG1, 0x2000);
	close(fd);
	return 0;	
}
