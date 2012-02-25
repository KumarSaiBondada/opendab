/*
    wfsleep.c

    Copyright (C) 2007  David Crawley

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
** Sleep function is here to simplify portability
**
*/
#include "opendab.h"

void wf_sleep(int usec) {

  struct timespec t;

  t.tv_sec = 0;
  t.tv_nsec = usec * 1000L;

  nanosleep(&t, (struct timespec *)NULL);
}
