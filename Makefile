OBJS = batracios.o 
SOURCES = batracios.c
HEADERS = batracios.h
CC = gcc
#CTAGS = -c
CTAGS = -c -g -DDEBUG

all : batracios

batracios : $(OBJS) libbatracios.a
	$(CC) -lm -o batracios $(OBJS) -L libbatracios.a

batracios.o : batracios.c
	$(CC) $(CTAGS) batracios.c

clean:
	rm $(OBJS) batracios