all:TestThread

CC=g++
CPPFLAGS=-Wall -std=c++11 -ggdb
LDFLAGS=-pthread

Thread:TestThread.o
	$(CC) $(LDFLAGS) -o $@ $^

Thread.o:TestThread.cpp
	$(CC) $(CPPFLAGS) -o $@ -c $^


.PHONY:
	clean

clean:
	rm TestThread.o testThread
