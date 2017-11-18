CC = gcc
CFLAGS = -g -w

all:: parallelCal vectorMultiply externalCal

parallelCal: 
	$(CC) $(CFLAGS) -pthread -o parallelCal parallelCal.c -L../ -lmy_pthread

vectorMultiply: 
	$(CC) $(CFLAGS) -pthread -o vectorMultiply vectorMultiply.c -L../ -lmy_pthread

externalCal: 
	$(CC) $(CFLAGS) -pthread -o externalCal externalCal.c -L../ -lmy_pthread

test2:
	$(CC) $(CFLAGS) -pthread -o test2 test2.c -L../ -lmy_pthread

clean:
	rm -rf test2 parallelCal vectorMultiply externalCal *.o ./record/
