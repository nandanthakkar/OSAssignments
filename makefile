C = gcc
CFLAGS = -g -c
AR = ar -rc
RANLIB = ranlib


Target: my_pthread.a

my_pthread.a: my_pthread.o memory.o
	$(AR) libmy_pthread.a my_pthread.o memory.o
	$(RANLIB) libmy_pthread.a

my_pthread.o: my_pthread_t.h memory.h
	$(CC)  $(CFLAGS) my_pthread.c

memory.o: memory.h
	$(CC)  $(CFLAGS) memory.c


clean:
	rm -rf testfile *.o *.a
