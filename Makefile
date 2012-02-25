#
# Makefile for OpenDAB
#


all:	drv progs

drv:	
	(cd driver; make)

progs:
	(cd src; make)

clean:
	(cd src; make clean)
	(cd driver; make clean)


