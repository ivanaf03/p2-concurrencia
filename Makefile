CC=gcc
CFLAGS=-Wall -g -O0
LIBS=-lcrypto -pthread
OBJS=options.o queue.o md5.o

PROGS=md5

all: $(PROGS)

%.o : %.c
	$(CC) $(CFLAGS) -c $<

md5: $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS) $(LIBS)

clean:
	rm -f $(PROGS) *.o *~

