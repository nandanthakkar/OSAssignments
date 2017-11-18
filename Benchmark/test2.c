#include <stdio.h>
#include <unistd.h>

#include <pthread.h>

#include "../my_pthread_t.h"


pthread_t *thread;

int thread_num=32;

//thread worker function
int function() {
  printf("i am a function\n");
	int i = 0;
	int * x[1000];
  for(i =0; i < 1000;i++){
  	x[i] = malloc(sizeof(int));

  	if(x[i] == NULL) {
    		printf("x is null\n");
  	}	
	printf("mallocing %d",i);
  }
   for(i =0; i < 1000; i++){
        //int* x = malloc(sizeof(int));
        *x[i] = i;
	printf("printing: %d", *x[i]);
	/*if(x == NULL) {
                printf("x is null\n");
        }*/
  }
  for(i =0; i < 1000;i++){
        free(x[i]);

        /*if(x[i] == NULL) {
                printf("x is null\n");
        }*/
        printf("freeing: %d",x[i]);
  }

}


int main(int argc, char const *argv[]) {
  int i=0;
  //initialize thread_num// initialize pthread_t
	thread = (pthread_t*)malloc(thread_num*sizeof(pthread_t));

  for (i = 0; i < thread_num; ++i){
		printf("creating thread %d", i);
		pthread_create(&thread[i], NULL, &function, NULL);
		printf("exiting thread %d", i);
  }
  return 0;
}
