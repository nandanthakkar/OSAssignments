#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "my_pthread_t.h"

typedef struct _pageNode {
	//Size of largest free mmeory segment in the page. If -1, used for a request that spanned multiple pages
	//Can't use for any other requests. If other negative number, no page in use
	int largestFreeMemory;
	int threadId;
	int pageId;
	//Negative means in swap file. Positive means in memory. Offsets start at 1 not 0
	int offset;
} pageNode;

typedef struct _memNode {
	char inUse;
	int size;
} memNode;

static char firstTimeMalloc=1;

static void* memory;
//NOTE:made this void* to get rid of warnings, can be changed back if needed - Steve 11-9-2017, 9:10AM


//4096 bytes
static int pageSize;

//stack size of each thead;
static int stackSize=64000;

//968 bytes
static int tcbSize=(sizeof(tcb));

static int threadLimit=32;

static int leftBlockPageNumber=512;

static int rightBlockPageNumber=24;

static int sharedRegionPageNumber=4;

static int freePagesBufferNumber=3;

static int totalPagesInMemory=2048;

static int TotalPagesMemoryAndSwapFile=6144;

static int totalBytes=25165824;

static int bytesMemory=8388608;

//Size of segment of memory dedicated to pageNodes and mallocs from the phtread library
static int osSize;

//Size of segment for mallocs from pthread library
static int OSLeftBlock;

//Size of segment for pageNodes
static int OSRightBlock;


//error function to print message and exit
void fatalError(int line, char* file) {
	printf("Error:\n");
	//TODO: add error message
	printf("Line number= %i \nFile= %s\n", line, file);
	exit(-1);
}

//NOTE: argument 2 changed to char* from int -Steve 11-9-2017 9:23AM
void * myallocate(size_t size, char* FILE, int LINE, int THREADREQ) {

	//first time malloc is called, need to initalize mmory array
	if(firstTimeMalloc==1) {

		//printf("Page Node Size: %d\n",sizeof(pageNode));
		//printf("Mem Node Size: %d\n",sizeof(memNode));
		//printf("TCB Size: %d\n",tcbSize);

		//Macro to get the page size
		pageSize=sysconf(_SC_PAGE_SIZE);

		int x=posix_memalign(&memory,pageSize,1048576);
		if(x != 0) {
			fatalError(__LINE__, __FILE__);
		}

		//Make sure this if statement never fires again
		firstTimeMalloc=0;

		//Create a node to manage the free space in the left block
		memNode OSNode1={0,leftBlockPageNumber*pageSize-sizeof(memNode)};

		//Copy that node into the memory array
		memcpy(memory,&OSNode1,sizeof(memNode));

		//Lef block shold be set up now
		//Left block contains memory allocated by the my_pthread library
		//Right block contains pageNodes

		//*****************************************************************

		//Set up right block
		pageNode mainPageNode= {1,0,0,1};
		memcpy(memory+leftBlockPageNumber*pageSize,&mainPageNode,sizeof(pageNode));

		//initialize actualt page with a memNode


	}
}


void main() {
	int j=7;

	myallocate(10, __FILE__, __LINE__, 0);

}
