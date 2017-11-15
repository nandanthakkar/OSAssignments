#ifndef MEMORY_H
#define MEMORY_H

#define _GNU_SOURCE

#define THREADREQ 1


typedef struct _pageNode {
	//Size of largest free memory segment in the page. If -1, used for a request that spanned multiple pages
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

//file descriptor
static int fd;

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

static int totalPagesUser=1505;

static int alwaysFreePages=3;

static int totalPagesMemoryAndSwapFile=6144;

static int totalBytes=25165824;

static int bytesMemory=8388608;


//Size of segment for mallocs from pthread library
static int OSLeftBlockSize;

//Size of segment for pageNodes
static int OSRightBlockSize;

//Size of segment of memory dedicated to pageNodes and mallocs from the phtread library
static int OSSize;

//Size of free memory when a new page is allocated
static int freshPageFreeSize;

static void* rightBlockStart;

static void* pageSpaceStart;




int memAlignPages();

void * myallocate(size_t size, char* file1, int line1 , int thread);

void mydeallocate(void* address, char* file1, int line1, int thread);

 #define malloc(x) myallocate(x, __FILE__, __LINE__, THREADREQ)
 #define free(x) mydeallocate(x, __FILE__, __LINE__, THREADREQ)

 #endif