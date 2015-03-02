all: main
OBJS = main.o
CC = g++
DEBUG = -g
CFLAGS = -Wall -O3 -c $(DEBUG) -std=c++11 -pthread
LFLAGS = -Wall $(DEBUG) -lpthread

main : $(OBJS)
	$(CC) $(OBJS) -o main $(LFLAGS)

main.o : block_sampler.h array.h
	$(CC) $(CFLAGS) main.cpp
	    
clean:
	rm -f *.o main