#include <stdio.h>
#include <unistd.h>

#include <pthread.h>

#include "../my_pthread_t.h"


pthread_t *thread;

int thread_num=32;

//thread worker function
int function() {
  printf("i am a function\n");
  int* x = malloc(sizeof(int));
  if(x == NULL) {
    printf("x is null\n");
  }
}


int main(int argc, char const *argv[]) {
  int i=0;
  //initialize thread_num// initialize pthread_t
	thread = (pthread_t*)malloc(thread_num*sizeof(pthread_t));

  for (i = 0; i < thread_num; ++i)
		pthread_create(&thread[i], NULL, &function, NULL);

  return 0;
}
