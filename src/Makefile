#
#  Makefile for OpenDAB
#
#  Copyright (C) 2008 David Crawley
#
#  This file is part of OpenDAB.
#
#  OpenDAB is free software: you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
#  OpenDAB is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with OpenDAB.  If not, see <http://www.gnu.org/licenses/>.


# Comment out the next line if you don't want support for DAB+.
#DABPLUS=1

CC=gcc
CFLAGS= -g -Wall -O3 -I /opt/local/include
LIBS=-lwfic -lfftw3 -lm
WFIC=./wfic
LIBPATH=-L $(WFIC) -L /opt/local/lib

ifdef DABPLUS
CFLAGS += -DDABPLUS
LIBS += -lfec
# Change this if you put libfec somewhere else
RUNPATH = -Xlinker -rpath -Xlinker /usr/local/lib
endif


COMMON_OBJS=	wfbyteops.o wfcrc.o wfdebug.o wfficproc.o wffigproc.o wffirecrc.o wfmdepunc.o \
		wfmsc.o wfsymsel.o wftables.o wfmp2.o wfdabplus.o

WF_OBJS=	wf.o wfafc.o wfclose.o wfcmd.o wfimp.o wfinit.o wfleds.o wfmaths.o \
		wfpk.o wfprsread.o wfravg.o wfread.o wfref.o wfscale.o wfsleep.o \
		wfsync.o wftiming.o wftune.o wfusb.o

WFX_OBJS=	wfx.o

#SRCS=   wfbyteops.c wfafc.c wfclose.c wfcmd.c  wfcrc.c wfdabplus.c wfdebug.c wfficproc.c wffigproc.c \
	wffirecrc.c wfimp.c wfinit.c wfleds.c wfmaths.c wfmdepunc.c wfmp2.c wfmsc.c wfpk.c wfprsread.c \
	wfravg.c wfread.c wfref.c wfscale.c wfsleep.c wfsymsel.c wfsync.c wftables.c \
	wftiming.c wftune.c wfusb.c

INCS= opendab.h wfsl11r.h $(WFIC)/wfic.h $(DRIVER)/wavefinder.h

all:	wfic wf wfx prsgen

wf:	$(WF_OBJS) $(COMMON_OBJS) $(INCS) $(WFIC)/libwfic.a
	$(CC) $(LIBPATH) $(RUNPATH) $(CFLAGS) $(WF_OBJS) $(COMMON_OBJS) -o wf $(LIBS)

wfx:	$(WFX_OBJS) $(COMMON_OBJS) $(INCS) $(WFIC)/libwfic.a
	$(CC) $(LIBPATH) $(RUNPATH) $(CFLAGS) $(WFX_OBJS) $(COMMON_OBJS) -o wfx $(LIBS)

opendab.h:

wfsl11r.h:

$(DRIVER)/wavefinder.h:

wfic:	$(WFIC)/libwfic.a

$(WFIC)/libwfic.a:
	(cd wfic; make)

prsgen: prsgen.c
	$(CC) $(CFLAGS) -g -o prsgen prsgen.c $(WFIC)/libwfic.a -L /opt/local/lib -lfftw3

clean:
	rm -f $(COMMON_OBJS) $(WF_OBJS) $(WFX_OBJS) wf wfx prsgen
	(cd wfic; make clean)
