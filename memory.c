#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "my_pthread_t.h"

typedef struct _pageNode {
	char inUse;
	int threadId;
	int pageId;
} pageNode;

typedef struct _memNode {
	char inUse;
	int size;
} memNode;

static char firstTimeMalloc=1;

//1 million bytes
static char memory[1000000];


//4096 bytes
static int pageSize;

//stack size of each thead;
static int stackSize=12000;

//968 bytes
static int tcbSize=(sizeof(tcb));

static int threadLimit=10;

static int pageLimit=10;

//Size of segment of memory dedicated to pageNodes and mallocs from the phtread library
static int osSize;

//Size of segment for mallocs from pthread library
static int OSLeftBlock;

//Size of segment for pageNodes
static int OSRightBlock;

void * myallocate(size_t size, int FILE, int LINE, int THREADREQ) {

	//first time malloc is called, need to initalize mmory array
	if(firstTimeMalloc==1) {

		//Macro to get the page size
		pageSize=sysconf(_SC_PAGE_SIZE);

		//Make sure this if statement never fires again
		firstTimeMalloc=0;

		//Calculate size of left and right block. Add the two to get the total os size
		OSLeftBlock=threadLimit*(sizeof(memNode)+stackSize+tcbSize);
		OSRightBlock=pageLimit*sizeof(pageNode);
		osSize=OSLeftBlock+OSRightBlock;

		//Create a node to manage the free space in the left block
		memNode OSNode1={0,OSLeftBlock-sizeof(memNode)};

		//Copy that node into the memory array
		memcpy(memory,&OSNode1,sizeof(memNode));

		//Lef block shold be set up now

		//****************


		//Set up right block

		pageNode mainPageNode= {1,0,0};

		memcpy(memory+OSLeftBlock,&mainPageNode,sizeof(pageNode));


	}

}


void main() {
	int j=1;
}
