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
	int * x[3000];
  for(i =0; i < 3000;i++){
  	x[i] = malloc(50*sizeof(long));

  	if(x[i] == NULL) {
    		printf("x is null\n");
  	}	
	//printf("mallocing %d\n",i);
  }
   for(i =0; i < 3000; i++){
        //int* x = malloc(sizeof(int));
        *x[i] = i;
	printf("printing: %d\n", *x[i]);
	/*if(x == NULL) {
                printf("x is null\n");
        }*/
  }
  for(i =0; i < 3000;i++){
        if(i != *x[i]){
		exit(-3);	
	}
  //printf("freeing: %d\n",x[i]);
	//free(x[i]);

        /*if(x[i] == NULL) {
                printf("x is null\n");
        }*/
  }

  my_pthread_join(1, NULL);

}


int main(int argc, char const *argv[]) {
  int i=0;
  //initialize thread_num// initialize pthread_t
	thread = (pthread_t*)malloc(thread_num*sizeof(pthread_t));

  for (i = 0; i < 15; ++i){
		printf("creating thread %d\n", i);
		pthread_create(&thread[i], NULL, &function, NULL);
		printf("exiting thread %d\n", i);
  }
  return 0;
}
