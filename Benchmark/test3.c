#include <stdio.h>
#include <unistd.h>

#include <pthread.h>

#include "../my_pthread_t.h"


pthread_t *thread;


int thread_num=32;

//thread worker function
int function(int num) {
        //my_pthread_join(1, NULL);

  printf("i am a function\n");
        int i = 0;
        int * x[100];
  for(i =0; i < 100;i++){
        x[i] = shalloc(sizeof(long));

        if(x[i] == NULL) {
                printf("x is null\n");
        }
        //printf("mallocing %d\n",i);
  }
   for(i =0; i < 100; i++){
        //int* x = malloc(sizeof(int));
        *x[i] = i;
        printf("printing: %d\n", *x[i]);
        /*if(x == NULL) {
                printf("x is null\n");
        }*/
  }
  for(i =0; i < 100;i++){
        if(i != *x[i]){
              printf("Error exiting\n");
                exit(-3);
        }
         printf("freeing: %ld\n",x[i]);
        free(x[i]);

        if(x[i] == NULL) {
                printf("x is null\n");
        }
  }

  printf("Exiting thread: %d\n", num);

}


int main(int argc, char const *argv[]) {
  int i=0;
  //initialize thread_num// initialize pthread_t
        thread = (pthread_t*)malloc(thread_num*sizeof(pthread_t));

  for (i = 0; i < 12; ++i){
                printf("creating thread %d\n", i);
                pthread_create(&thread[i], NULL, &function, i);
  }
  printf("Before joins\n");
  int j=0;
  for(j=2;j<14;j++) {
    my_pthread_join(j, NULL);
  }
  return 0;
}
