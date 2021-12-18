
CC=gcc
CFLAGS=-g -Wno-implicit -Wno-unused

all: parent.o passenger.o arrival.o officer.o hall.o bus.o checker.o

%.o: %.c
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -f *.o
	ipcrm --all
