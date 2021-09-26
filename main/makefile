#
# Makefile for triangleramsey
#

SHELL = /bin/sh

# Executing the program with DWORDSIZE=32 is slightly faster, 
# but limits the order of the graphs to 32.
CC = gcc -DMAXN=1024 -DWORDSIZE=32
CC64 = gcc -DMAXN=1024 -DWORDSIZE=64
CFLAGS = -O4

all : clean default

default :
	${CC} $(CFLAGS) nauty.c nautil.c naugraph.c schreier.c naurng.c triangleramsey.c -o triangleramsey

64bit :
	rm -rf triangleramsey-64
	${CC64} $(CFLAGS) nauty.c nautil.c naugraph.c schreier.c naurng.c triangleramsey.c -o triangleramsey-64

prof :
	rm -rf triangleramsey-prof gmon.out
	${CC} -pg -g nauty.c nautil.c naugraph.c schreier.c naurng.c triangleramsey.c -o triangleramsey-prof

clean :
	rm -f triangleramsey
