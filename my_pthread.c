// File:	my_pthread.c
// Author:	Yujie REN
// Date:	09/23/2017

// name:
// username of iLab:
// iLab Server:

#include "my_pthread_t.h"
#include "memory.h"
#include <stdio.h>
#include <stdlib.h>
#include <ucontext.h>
#include <signal.h>
#include <sys/time.h>
#include <time.h>
#include <errno.h>
#include <unistd.h>
#define SIZE 32000
#define TIME 25000

//globals
static int i=1;
static tcb* head=NULL;                   //head of the tcb
static tcb* waitQueueHead=NULL;
static tcb* fifoHead=NULL;
static ucontext_t schedulerContext;      //context for scheduler, this may not be necessary
static my_pthread_t currentThread=1;
static struct itimerval iValue;
static time_t lastMaintenance=0;
static tcb* nextThread=NULL;
static char cursorEqualsFifoHead=0;
static char inComplete=0;


//function decleration
void schedule(int x);

void maintenance();


int getCurrentThread() {
  return (int)currentThread;
}


/******************************************************************************/
//signal handeler function to handle the signal
void signalhandeler(int x) {
  //printf("signalhandeler\n");
  
  if(inComplete == 1||getInMemoryCFile()==1) {
	  return;
  }

  time_t currentTime=time(NULL);

  tcb* ptr2=head;
  tcb* currentThreadTCB2;
  while(ptr2!=NULL) {
    if(ptr2->thread==currentThread) {
      currentThreadTCB2=ptr2;
      break;
    }

    ptr2=ptr2->next;
  }


  //check maintenance cycle
  if((currentTime - lastMaintenance) >= TIME / 25000) {
    maintenance();
  }

  tcb* ptr=head;
  tcb* currentThreadTCB;
  while(ptr!=NULL) {
  	if(ptr->thread==currentThread) {
  		currentThreadTCB=ptr;
  		break;
  	}

  	ptr=ptr->next;
  }

  ptr=fifoHead;
  while(ptr!=NULL) {
  	if(ptr->thread==currentThread) {
  		currentThreadTCB=ptr;
  		break;
  	}

  	ptr=ptr->next;
  }

  //swap back to scheduler, scheduler takes care of rest
  swapcontext(&currentThreadTCB->context, &schedulerContext);
}



/****************************************************************************/
void schedule(int x){


  my_pthread_t schedulerThread;
  tcb* cursor;
  int i=0;

  //install signal handeler and i timer
  iValue.it_interval.tv_sec=0;
  iValue.it_interval.tv_usec=0;

  iValue.it_value.tv_usec=TIME;

  signal(SIGALRM, signalhandeler);

  cursor=head;
  //skip main since it just ran
  cursor=cursor->next;
  while(1) {
    currentThread=cursor->thread;
    memAlignPages();
    //printf("back to scheduler from memAlignPages\n");
    getcontext(&schedulerContext);
    nextThread=cursor->next;
    if(cursor==fifoHead) {
      cursorEqualsFifoHead=1;
    } else {
      cursorEqualsFifoHead=0;
    }
    setitimer(ITIMER_REAL, &iValue, NULL);
    //printf("Before Swap to : %d\n",cursor->thread);
    swapcontext(&schedulerContext, &cursor->context);
   // printf("back to scheduler\n");
    currentThread=schedulerThread;
    if(head != NULL) {
      if(nextThread!= NULL && cursorEqualsFifoHead==0) {
        cursor=nextThread;
      }
      else {
        cursor=head;
      }
    }
    else {
      cursor=fifoHead;
    }
}
}



/***************************************************************************************/
void maintenance() {
	tcb* removedFromRR=NULL;
	tcb* removedFromFIFO=NULL;
	tcb* cursor=head;
	tcb* prev=NULL;

	//max time we want a thread to stay in each queue
	int cycle=1;

	//set time
	time_t currentTime=time(NULL);

	//check round robin list
	while(cursor != NULL) {
		if((currentTime - cursor->onQueue) > cycle) {
			//3 casses
			if(prev == NULL) {
				head=cursor->next;
				cursor->onQueue=0;

        cursor->next=removedFromRR;
        removedFromRR=cursor;

        cursor=head;
        continue;
			}
			//handeles 2 of the three casses
			else {
				prev->next=cursor->next;
				cursor->onQueue=0;

        cursor->next=removedFromRR;
        removedFromRR=cursor;
        cursor=prev->next;

        continue;
			}
		}
		prev=cursor;
		cursor=cursor->next;
	}

	//check FIFO list
	prev=NULL;
	cursor=fifoHead;
	while(cursor != NULL) {
		if((currentTime - cursor->onQueue) > cycle) {
			//3 casses
			if(prev == NULL) {
				fifoHead=cursor->next;
				cursor->onQueue=0;

        cursor->next=removedFromFIFO;
        removedFromFIFO=cursor;

        cursor=fifoHead;
        continue;
			}
			//handeles 2 of the three casses
			else {
				prev->next=cursor->next;
				cursor->onQueue=0;

        cursor->next=removedFromFIFO;
        removedFromFIFO=cursor;

        cursor=prev->next;
			}
		}

    prev=cursor;
    cursor=cursor->next;
	}

	//check removed from RR
	if(removedFromRR != NULL) {
		cursor=fifoHead;
		if(cursor==NULL) {
			fifoHead=removedFromRR;
		}
		else {
			while(cursor->next != NULL) {
				cursor=cursor->next;
			}
			cursor->next=removedFromRR;
		}
	}

	//check removed from FIFO
	if(removedFromFIFO != NULL) {
		cursor=head;
		if(cursor == NULL) {
			head=removedFromFIFO;
		}
		else {
			while(cursor->next != NULL) {
				cursor=cursor->next;
			}
			cursor->next=removedFromFIFO;
		}
	}

}



/*****************************************************************************************/
//removes a thread from TCB
void threadComplete() {
  inComplete=1;
  int prevThread=currentThread;
 // printf("Thread complete called on: %d\n", prevThread);
 // currentThread=-8;

  tcb* cursor;
  tcb* temp;
  tcb* prev;
  tcb* addToRRHead;
  cursor=waitQueueHead;
  prev=NULL;

  while(cursor!=NULL) {
  	if(cursor->waitId==prevThread) {
  		tcb* temp2=cursor;
  		if(prev==NULL) {
  			waitQueueHead=cursor->next;
  		} else {
  			prev->next=cursor->next;
  		}
  		temp2->waitId=-1;
  		if(addToRRHead==NULL) {
  			addToRRHead=temp2;
  		} else {
  			temp2->next=addToRRHead;
  			addToRRHead=temp2;
  		}
  	}
  	prev=cursor;
  	cursor=cursor->next;
  }

  cursor=head;
  prev=NULL;
  if(cursor!=NULL) {
  	if(cursor->thread==prevThread) {
  		temp=cursor;
  		head=cursor->next;
      if(head==NULL) {
        head=addToRRHead;
      } else {
        cursor=head;
        while(cursor->next!=NULL) {
          cursor=cursor->next;
        }

        cursor->next=addToRRHead;
      }
  	}
    else {
      prev=cursor;
      cursor=cursor->next;
      while(cursor!=NULL) {
        if(cursor->thread==prevThread) {
          temp=cursor;
          prev->next=cursor->next;
          cursor=prev->next;
          continue;
        }
        prev=cursor;
        cursor=cursor->next;
      }
      prev->next=addToRRHead;
  	}
  } else {
  	head=addToRRHead;
  }

  cursor=fifoHead;
  prev=NULL;
  if(cursor!=NULL) {
  	if(cursor->thread==prevThread) {
  		temp=cursor;
  		fifoHead=cursor->next;
  	}

  	else {
      prev=cursor;
      cursor=cursor->next;
      while(cursor!=NULL) {
        if(cursor->thread==prevThread) {
          temp=cursor;
          prev->next=cursor->next;
          cursor=prev->next;
          continue;
        }
        prev=cursor;
        cursor=cursor->next;
      }
  	}
  }
  //printf("about to clean ot pages\n");
  cleanOutPageNodes(prevThread);
 // printf("about to free\n");
  free((temp->context).uc_stack.ss_sp);
  free(temp);
  ucontext_t tempContext;
  getcontext(&tempContext);
  inComplete=0;
  swapcontext(&tempContext,&schedulerContext);
}



/*****************************************************************************************************/
/* create a new thread */
int my_pthread_create(my_pthread_t * thread, pthread_attr_t * attr, void *(*function)(void*), void * arg) {
  tcb* cursor;
  tcb* temp;
  ucontext_t newContext;
  ucontext_t mainContext;
  ucontext_t completeContext;       //context for thread to go when it completes
  my_pthread_t* main;

  //printf("CREATE CALLED\n");


  //build new context
  getcontext(&newContext);
  newContext.uc_link=&completeContext;
  newContext.uc_stack.ss_sp=myallocate(SIZE,__FILE__,__LINE__,0);
  if(newContext.uc_stack.ss_sp == NULL) {
    return ENOMEM;
  }
  //printf("new context stack allocated\n");


  newContext.uc_stack.ss_size=SIZE;
  makecontext(&newContext, (void*)function, 1, arg);

  //build context for thread return
  getcontext(&completeContext);
  completeContext.uc_link=&schedulerContext;
  completeContext.uc_stack.ss_sp=myallocate(SIZE,__FILE__,__LINE__,0);
  if(completeContext.uc_stack.ss_sp == NULL) {
    return ENOMEM;
  }

 // printf("complete stack allocated\n");



  completeContext.uc_stack.ss_size=SIZE;
  makecontext(&completeContext, (void*)threadComplete, 1, arg);

  //now check if first call to create
  if(i==1) {
    //build context for scheduler
    getcontext(&schedulerContext);
    schedulerContext.uc_link=NULL;
    schedulerContext.uc_stack.ss_sp=myallocate(SIZE,__FILE__,__LINE__,0);
    //printf("scheduler context loc: %lu\n", schedulerContext.uc_stack.ss_sp);
    if(schedulerContext.uc_stack.ss_sp == NULL) {
      return ENOMEM;
    }
    schedulerContext.uc_stack.ss_size=SIZE;
    makecontext(&schedulerContext, (void*)schedule, 1, arg);

     //printf("scheduler stack allocated\n");


    //build context for main
    getcontext(&mainContext);
    mainContext.uc_link=&schedulerContext;
    mainContext.uc_stack.ss_sp=myallocate(SIZE,__FILE__,__LINE__,0);
    printf("main stack loc: %lu\n",mainContext.uc_stack.ss_sp);
    if(mainContext.uc_stack.ss_sp == NULL) {
      return ENOMEM;
    }
    mainContext.uc_stack.ss_size=SIZE;

    //allocate space for main
    head=(tcb*)myallocate(sizeof(tcb),__FILE__,__LINE__,0);
    if(head==NULL) {
      return ENOMEM;
    }
   // printf("head allocated\n");
    head->next=(tcb*)myallocate(sizeof(tcb),__FILE__,__LINE__,0);
    if(head->next == NULL) {
      return ENOMEM;
    }

    //printf("head next allocated\n");
    head->context=mainContext;
    head->waitId=-1;
    head->thread=i;
    i++;

    cursor=head;
    cursor=cursor->next;
    cursor->next=NULL;
    cursor->context=newContext;
    cursor->onQueue=time(NULL);
    cursor->waitId=-1;
    cursor->thread=i;
    *thread=i;
    i++;

    swapcontext(&head->context, &schedulerContext);
  }
  else {
    //add new thread right after main thread in linked list
    cursor=head;
    temp=(tcb*)myallocate(sizeof(tcb),__FILE__,__LINE__,0);
    if(temp == NULL) {
      return ENOMEM;
    }
    //printf("temp allocated\n");
    memset(temp,0,sizeof(tcb));
    temp->next==NULL;
    if(head==NULL) {
      head=temp;
      temp->next==NULL;
    } else {
      while(cursor->next!=NULL) {
        //printf("Loop pthread 1\n");
        cursor=cursor->next;
      }
      cursor->next=temp;
      temp->next==NULL;
    }

    temp->context=newContext;
    temp->thread=i;
    temp->onQueue=time(NULL);
    temp->waitId=-1;
    *thread=i;
    i++;

    cursor=head;
    tcb* mainThread=NULL;
    while(cursor!=NULL) {
      //printf("Loop pthread 2\n");
      if(cursor->thread==1) {
        mainThread=cursor;
        break;
      }

      cursor=cursor->next;
    }

   // printf("out of loop2 \n");

    if(mainThread==NULL) {
      cursor=fifoHead;
      while(cursor!=NULL) {
      //  printf("Loop pthread 3\n");
        if(cursor->thread==1) {
          mainThread=cursor;
          break;
        }

      cursor=cursor->next;
     }
    }


    //swap to scheduler
    swapcontext(&mainThread->context, &schedulerContext);
  }
  return 0;
};

/* give CPU pocession to other user level threads voluntarily */
int my_pthread_yield() {
	tcb* ptr=head;
	while(ptr!=NULL) {
		if(currentThread==ptr->thread) {
			swapcontext(&ptr->context,&schedulerContext);
			return 0;
		}
		ptr=ptr->next;
	}

	ptr=fifoHead;
	while(ptr!=NULL) {
		if(currentThread==ptr->thread) {
			swapcontext(&ptr->context,&schedulerContext);
			return 0;
		}
		ptr=ptr->next;
	}

  return 0;
}

/* terminate a thread */
void my_pthread_exit(void *value_ptr) {
  tcb* cursor=head;
  tcb* innerCursor=waitQueueHead;
  ucontext_t currentContext;

  getcontext(&currentContext);

  //check round robin
  while(cursor != NULL) {
    if(cursor->thread == currentThread){
      while(innerCursor != NULL) {
        if(innerCursor->waitId==cursor->thread) {
          innerCursor->returnValue=value_ptr;
        }
        innerCursor=innerCursor->next;
      }
      break;
    }
    cursor=cursor->next;
  }

  cursor=fifoHead;
  innerCursor=waitQueueHead; 
  while(cursor != NULL) {
    if(cursor->thread == currentThread){
      while(innerCursor != NULL) {
        if(innerCursor->waitId==cursor->thread) {
          innerCursor->returnValue=value_ptr;
        }
        innerCursor=innerCursor->next;
      }
      break;
    }
    cursor=cursor->next;
  }
  threadComplete();

  swapcontext(&currentContext, &schedulerContext);

};



/**************************************************************************************/
/* wait for thread termination */
int my_pthread_join(my_pthread_t thread, void **value_ptr) {
  //waits for certain thread to finish
  int found=0;
  tcb* cursor=head;
  tcb* current=NULL;


  //check if thread exists
  cursor=head;
  while(cursor != NULL) {
    if(cursor->thread==thread) {
      found=1;
    }
    cursor=cursor->next;
  }


  //check if thread exists in fifo
  cursor=fifoHead;
  while(cursor != NULL) {
    if(cursor->thread==thread) {
      found=1;
    }
    cursor=cursor->next;
  }


  if(found == 0) {
    cursor=head;
    while(cursor!=NULL) {
      if(cursor->thread==currentThread) {
        current=cursor;
        break;
      }
      cursor=cursor->next;
    }
    
    cursor=fifoHead;
    while(cursor!=NULL) {
      if(cursor->thread==currentThread) {
        current=cursor;
        break;
      }
      cursor=cursor->next;
    }

    if(current->returnValue!=NULL) {
        *value_ptr=current->returnValue;
       }

    return 0;
  }


  cursor=head;
  tcb* prev=NULL;

  while(cursor!=NULL) {
    if(cursor-> thread == currentThread) {
        cursor->waitId=thread;
        current=cursor;

        if(prev==NULL) {
          head=cursor->next;
        } else {
          prev->next=cursor->next;
        }
        current->next=NULL;
        break;
      }

      prev=cursor;
      cursor=cursor->next;
  }

  cursor=fifoHead;
  prev=NULL;
  while(cursor!=NULL) {
    if(cursor-> thread == currentThread) {
        cursor->waitId=thread;
        current=cursor;

        if(prev==NULL) {
          head=cursor->next;
        } else {
          prev->next=cursor->next;
        }
        current->next=NULL;
        break;
      }

      prev=cursor;
      cursor=cursor->next;
  }

  
 cursor=waitQueueHead;
 if(waitQueueHead==NULL) {
 	waitQueueHead=current;
 } else {
 	while(cursor->next!=NULL) {
 		cursor=cursor->next;
 	}
 	cursor->next=current;
 }

 if(current->returnValue!=NULL) {
  *value_ptr=current->returnValue;
 }

 //swap back to scheduler
 swapcontext(&current->context, &schedulerContext);
 if(current->returnValue != NULL) {
   *value_ptr=current->returnValue;
 }
 return 0;
};

/* initial the mutex lock */
int my_pthread_mutex_init(my_pthread_mutex_t *mutex, const pthread_mutexattr_t *mutexattr) {


  if(mutex == NULL)
    return EINVAL;  

  mutex->wait_mutex = 0;    
  return 0;
};

/* aquire the mutex lock */
int my_pthread_mutex_lock(my_pthread_mutex_t *mutex) {

  
  if(mutex == NULL)
    return EINVAL;
  while((__sync_lock_test_and_set(&(mutex->wait_mutex),1))) {
      my_pthread_yield();
    }
  
  return 0;
}

/* release the mutex lock */
int my_pthread_mutex_unlock(my_pthread_mutex_t *mutex) {

  __sync_lock_release(&(mutex->wait_mutex));    
  return 0;
};

/* destroy the mutex */
int my_pthread_mutex_destroy(my_pthread_mutex_t *mutex) {
    
  if(mutex == NULL)
    return EINVAL;

  if(mutex->wait_mutex != 0){         //Cannot destroy a locked mutex
    return EBUSY;
  }

  else{
    mutex = NULL;     //Destroy mutex 
  }
  
  return 0;
};