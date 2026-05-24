CC = gcc
CFLAGS = -Wall -Wextra -g

all: tarsau

tarsau: main.o tarsau.o tarac.o
	$(CC) $(CFLAGS) -o tarsau main.o tarsau.o tarac.o

main.o: main.c tarsau.h tarac.h
	$(CC) $(CFLAGS) -c main.c

tarsau.o: tarsau.c tarsau.h
	$(CC) $(CFLAGS) -c tarsau.c

tarac.o: tarac.c tarac.h
	$(CC) $(CFLAGS) -c tarac.c

clean:
	rm -f *.o tarsau