OBJS = batracios.o 
SOURCES = batracios.c
HEADERS = batracios.h
CC = gcc
#CTAGS = -c
CTAGS = -c -g -DDEBUG

all : batracios

batracios : $(OBJS) libbatracios.a
	$(CC) -m32 $(OBJS) libbatracios.a -o batracios -lm

batracios.o : batracios.c
	$(CC) -m32 $(CTAGS) batracios.c

clean:
	rm $(OBJS) batracios
