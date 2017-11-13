#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "my_pthread_t.h"

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

static int totalPagesUser=1509;

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


//function to swap pages from memory to file
int swap(pageNode* toSwap, pageNode* toMemory) {
	
}

//error function to print message and exit
void fatalError(int line, char* file) {
	printf("Error:\n");
	//TODO: add error message
	printf("Line number= %i \nFile= %s\n", line, file);
	exit(-1);
}

int memAlignPages(int threadID){
	//count pages align
	pageidCounter = 1;
	
	//temp pageNode to find all pages of given thread
	_pageNode * pageNodeAddr = (_pageNode *)rightBlockStart;
	//loop to scroll through all page nodes
	while(pageNodeAddr < (char *)rightBlockStart + OSRightBlockSize){
		// if found page of a thread then align it.
		if(pageNodeAddr->threadId = threadID && pageidCounter == pageNodeAddr->pageId){
			pageidCounter++;
			//if page node we  need to align is in memory
			if(pageNodeAddr->offset > 0){
				
				//create temp page node to find if space where we want align if free or occupied
				_pageNode * pageTemp = (_pageNode *)rightBlockStart;
				
				// created to find if space where we want to align  is occupied.
				char isOccupied = 0;
				
				//find page node for space where we want to align
				while(pageTemp < (char *)rightBlockStart + OSRightBlockSize){
					
					// if found a
					if(pageTemp->offset == ((pageNodeAddr->pageId)+1)){
						memcpy((void*)(memory + bytesMemory - pageSize*7), (void*)(memory + ((pageTemp->offset) - 1) * pageSize), pageSize);
						memcpy((void*)(memory + ((pageTemp->offset) - 1) * pageSize), (void*)(memory + ((pageNodeAddr->offset) * - 1) * pageSize), pageSize);
						memcpy((void*)(memory + ((pageNodeAddr->offset) - 1) * pageSize),(void*)(memory + bytesMemory - pageSize*7), pageSize);
						memset((void*)(memory + bytesMemory - pageSize*7), NULL, pageSize);
						int offsetTemp = pageTemp->offset;
						pageTemp->offset = pageNodeAddr->offset;
						pageNodeAddr->offset = offsetTemp;
						isOccupied = 1;
					}
					
					pageTemp++;
				}
				
				if(!isOccupied){
					memcpy((void*)(memory + (pageNodeAddr->pageId - 1) * pageSize),(void*)(memory + pageNodeAddr->offset*(-1) + 1));
				}
			}
			
			if(pageNodeAddr->offset < 0){
				_pageNode * pageTemp = (_pageNode *)rightBlockStart;
				char isOccupied = 0;
				while(pageTemp < (char *)rightBlockStart + OSRightBlockSize){
					if(pageTemp->offset == ((pageNodeAddr->pageId)*(-1)-1)){
						int i = swap(pageNodeAddr, pageTemp);
						if(i == 0){
							perror("swap did not work");
							return i;
						}
						isOccupied = 1;
					}
					pageTemp++;
				}
				
				if(!isOccupied){
					int i = swap(pageNodeAddr, NULL);
					if(i == 0){
						perror("swap did not work");
						return i;
					}
				}
			}
		}
		pageNodeAddr++;
	}
	return 1;
}

void* allocateMemoryInPage(size_t size, pageNode* pageNodePtr);

//NOTE: argument 2 changed to char* from int -Steve 11-9-2017 9:23AM
void * myallocate(size_t size, char* FILE, int LINE, int THREADREQ) {

	//first time malloc is called, need to initalize mmory array
	if(firstTimeMalloc==1) {

		//Macro to get the page size
		pageSize=sysconf(_SC_PAGE_SIZE);

		int x=posix_memalign(&memory,pageSize,bytesMemory);
		if(x != 0) {
			fatalError(__LINE__, __FILE__);
		}


		OSLeftBlockSize=leftBlockPageNumber*pageSize;
		OSRightBlockSize=rightBlockPageNumber*pageSize;
		OSSize=OSLeftBlockSize+OSRightBlockSize;
		freshPageFreeSize=pageSize-sizeof(memNode);
		rightBlockStart=memory+leftBlockPageNumber*pageSize;
		pageSpaceStart=rightBlockStart+rightBlockPageNumber*pageSize;


		//Make sure this if statement never fires again
		firstTimeMalloc=0;

		//Create a node to manage the free space in the left block
		memNode OSNode1={0,leftBlockPageNumber*pageSize-sizeof(memNode)};

		//Copy that node into the memory array
		*((memNode*)memory)=OSNode1;

		//Lef block shold be set up now
		//Left block contains memory allocated by the my_pthread library
		//Right block contains pageNodes

		//*****************************************************************

		pageNode* currentPageNode=(pageNode*)rightBlockStart;


		//Set up right block
		pageNode mainPageNode= {freshPageFreeSize,1,1,1};
		*currentPageNode=mainPageNode;

		//initialize actual page with a memNode
		memNode* currentPage=(memNode*)pageSpaceStart;
		memNode firstPageMemNode={0,freshPageFreeSize};
		*currentPage=firstPageMemNode;


		int pageNodeCreatedCounter=1;

		//adress of the current page node to be created
		currentPageNode+=1;


		currentPage=(memNode*)((char*)currentPage+pageSize);


		//loop through the pageNodes for pages in memory and initalize the pageNodes as well as their pages
		while(pageNodeCreatedCounter<totalPagesUser) {

			pageNode newPageNode={-2,0,0,1+pageSize*pageNodeCreatedCounter};
			*currentPageNode=newPageNode;

			memNode newMemNode={0,freshPageFreeSize};
			*currentPage=newMemNode;


			currentPage=(memNode*)((char*)currentPage+pageSize);
			currentPageNode+=1;
			pageNodeCreatedCounter++;

		}

		int numNodesInMemory=pageNodeCreatedCounter;

		//Initialize the pageNodes of pages not in memory
		while(pageNodeCreatedCounter<totalPagesMemoryAndSwapFile - totalPagesUser) {
			pageNode newPageNode={-2,0,0,-1 - (pageNodeCreatedCounter-numNodesInMemory)*pageSize};
			*currentPageNode=newPageNode;

			currentPageNode+=1;
			pageNodeCreatedCounter++;
		}

	}

	//Library called malloc
	if(THREADREQ==0) {

		//The start of the os left block
		memNode* cursor=(memNode*)memory;

		//Loop through the entire left block
		while(cursor<(memNode*)(memory+OSLeftBlockSize)) {

			//If the block is being used skip
			if(cursor->inUse==1) {
				cursor=(memNode*)((char*)cursor+cursor->size+sizeof(memNode));
				continue;
			}

			//origina size that was available
			int originalSize=cursor->size;

			//If there is room for the allocation plus a memNode with a size of 1
			if(originalSize>=size+sizeof(memNode)+1) {

				//Update cursor to refer to the new allocation
				cursor->size=size;
				cursor->inUse=1;

				//Create a new memode for remaining free space
				memNode* newMemNodePtr=(memNode*)((char*)cursor+sizeof(memNode)+size);
				newMemNodePtr->size=originalSize-size-sizeof(memNode);
				newMemNodePtr->inUse=0;

				//return the adress of the allocation
				return (void*)(cursor+1);
			}

			//If there is only room for the allocation update the memNode to refer to the new allocation and return the adress
			//of the new allocation
			else if(originalSize>=size){
				cursor->inUse=1;
				return (void *) (cursor+1);
			}

			//jump to the next memNode
			cursor=(memNode*)((char*)cursor+sizeof(memNode)+cursor->size);
		}

		//If no space available return NULL
		return NULL;

	}

	//User made call to malloc
	else {

		//Start of pageNodes
		pageNode* currentPageNode=(pageNode*)rightBlockStart;

		//Count of pageNodes seen with corresponding threadId
		int pageCount=0;

		//Count of all pages seen
		int pageCounter=0;

		//If the allocation can fit inside one page
		if(size<=freshPageFreeSize) {

			//Loop through all pageNodes
			while(pageCounter<totalPagesMemoryAndSwapFile) {

				//If you find a pageNode with correspoding threadId
				if(currentPageNode->threadId==getCurrentThread()) {

					//In memory and there is space available in the page
					if(currentPageNode->offset>0&&currentPageNode->largestFreeMemory>=size) {

						//allocate the memory
						return pageSpaceStart+(currentPageNode->pageId-1)*pageSize+(long)(allocateMemoryInPage(size,currentPageNode)-currentPageNode->offset);
					}

					//In swap file and there is space avaible
					else if (currentPageNode->offset<0&&currentPageNode->largestFreeMemory>=size) {
						//bring page from swap file into memory

						//allocte the memory
						return pageSpaceStart+(currentPageNode->pageId-1)*pageSize+(long)(allocateMemoryInPage(size,currentPageNode)-currentPageNode->offset);
					}

					//Page count needs to be incremented
					pageCount++;
				}

				//Jump to the next page node
				currentPageNode+=1;

				//Increment the number of pages seen
				pageCounter++;
			}

			if(pageCount>=totalPagesUser) {
				return NULL;
			}

			//Re-initialize these variable since loop starts from beginning again
			pageCounter=0;
			currentPageNode=(pageNode*)rightBlockStart;

			//Loop through all pageNodes
			while(pageCounter<totalPagesMemoryAndSwapFile - totalPagesUser - alwaysFreePages) {

				//pageNode is free
				if(currentPageNode->threadId==0) {

					//Set its thread, pageId, and largestFreeMemory
					currentPageNode->threadId=getCurrentThread();
					currentPageNode->pageId=pageCount;
					currentPageNode->largestFreeMemory=freshPageFreeSize;

					//If the pageNode refers to a page not in memory than bring it into memory and initialize the page
					if(currentPageNode->offset<0) {
						//bring page form swap file to memory
						//initialize page
					}

					//allocate the memory
					return pageSpaceStart+(currentPageNode->pageId-1)*pageSize+(long)(allocateMemoryInPage(size,currentPageNode)-currentPageNode->offset);
				}

			}

			//If all pageNodes used, no more memory to allocate
			return NULL;
		}

		//Request will span two or more pages
		else {

			//Last Page Node allocated to the thread
			pageNode* highestPageId=NULL;

			//Find the highest pageId allocated to the thread
			while(pageCounter<totalPagesMemoryAndSwapFile) {
				if(currentPageNode->threadId==getCurrentThread()) {
					if(highestPageId==NULL||currentPageNode->pageId>highestPageId->threadId) {
						highestPageId=currentPageNode;
					}
				}

				currentPageNode+=1;
				pageCounter++;
			}



			//No page has been allocated to the thread
			if(highestPageId==NULL) {

				//The number of pages thar are going to have to be allocated for the request
				int newPagesNum=(size+sizeof(memNode))/pageSize;

				//If the total request won't be able to fit into memory at one time return null
				if(newPagesNum>totalPagesUser) {
					return NULL;
				}

				//Array of pageNode* to hold the free pageNodes that are found
				pageNode* freeNodes[newPagesNum];

				//Counter for the while loop
				int pageCounter=0;

				//Current page node
				pageNode* currentPageNode=(pageNode*)rightBlockStart;

				//index for the array of pageNode*
				int index=0;

				//loop through all the pageNodes
				while(pageCounter<totalPagesMemoryAndSwapFile - totalPagesUser - alwaysFreePages) {

					//If a free pageNode is found, save it to the array
					if(currentPageNode->largestFreeMemory<-1) {
						freeNodes[index]=currentPageNode;
						index++;
					}

					//If the array is full break
					if(index>=newPagesNum) {
						break;
					}


					currentPageNode=currentPageNode+1;
					pageCounter++;
				}

				//Not enough memory left for request reurn null
				if(index<newPagesNum) {
					return NULL;
				}

				//The first page node is special because its page holds a memNode
				pageNode* firstPageNode=freeNodes[0];

				//Initialize the pageNode
				firstPageNode->threadId=getCurrentThread();
				firstPageNode->pageId=1;
				firstPageNode->largestFreeMemory=-1;

				//If the page is not in memory bring it into memory
				if(firstPageNode->offset<0) {
					//swap into memory
				}

				//Start of the page in memory
				memNode* startOfPage=(memNode*)((char*)pageSpaceStart+firstPageNode->offset);

				//Initialize the memNode
				startOfPage->inUse=1;
				startOfPage->size=size;

				int i=1;

				//Initialize  the rest of the memNodes and bring them into memory if necessary
				for(i;i<newPagesNum;i++) {
					pageNode* pgNode=freeNodes[i];
					pgNode->threadId=getCurrentThread();
					pgNode->largestFreeMemory=-1;
					pgNode->pageId=i+1;

					if(pgNode->offset<0) {
						//swap into memory
					}
				}

				//Return the address of the first page in memory
				return pageSpaceStart+sizeof(memNode);



			}

			else {
				int newPagesNum=(size+sizeof(memNode))/pageSize;

				if(newPagesNum+highestPageId->pageId>totalPagesUser) {
					return NULL;
				}


				//Array of pageNode* to hold the free pageNodes that are found
				pageNode* freeNodes[newPagesNum];

				//Counter for the while loop
				int pageCounter=0;

				//Current page node
				pageNode* currentPageNode=(pageNode*)rightBlockStart;

				//index for the array of pageNode*
				int index=0;

				//loop through all the pageNodes
				while(pageCounter<totalPagesMemoryAndSwapFile - totalPagesUser - alwaysFreePages) {

					//If a free pageNode is found, save it to the array
					if(currentPageNode->largestFreeMemory<-1) {
						freeNodes[index]=currentPageNode;
						index++;
					}

					//If the array is full break
					if(index>=newPagesNum) {
						break;
					}


					currentPageNode=currentPageNode+1;
					pageCounter++;
				}

				//Not enough memory left for request reurn null
				if(index<newPagesNum) {
					return NULL;
				}

				//The first page node is special because its page holds a memNode
				pageNode* firstPageNode=freeNodes[0];

				//Initialize the pageNode
				firstPageNode->threadId=getCurrentThread();
				firstPageNode->pageId=highestPageId->pageId+1;
				firstPageNode->largestFreeMemory=-1;

				//If the page is not in memory bring it into memory
				if(firstPageNode->offset<0) {
					//swap into memory
				}

				//Start of the page in memory
				memNode* startOfPage=(memNode*)((char*)pageSpaceStart+firstPageNode->offset);

				//Initialize the memNode
				startOfPage->inUse=1;
				startOfPage->size=size;

				int i=1;

				//Initialize  the rest of the memNodes and bring them into memory if necessary
				for(i;i<newPagesNum;i++) {
					pageNode* pgNode=freeNodes[i];
					pgNode->threadId=getCurrentThread();
					pgNode->largestFreeMemory=-1;
					pgNode->pageId=highestPageId->pageId+1;
					if(pgNode->offset<0) {
						//swap into memory
					}
				}

				return pageSpaceStart+(pageSize-1)*firstPageNode->pageId+sizeof(memNode);

			}

		}

	}
}

/*
	Function to service a malloc request after the correct page has already been identified
*/
void* allocateMemoryInPage(size_t size, pageNode* pageNodePtr) {

	//location of beginning of page
	memNode* cursor=(memNode*)pageSpaceStart+pageNodePtr->offset;

	//location of end of page
	void* endOfPage=(void*)cursor+pageSize;


	while(cursor<(memNode*)endOfPage) {

			//If the memNode is in use skip
			if(cursor->inUse==1) {
				cursor=(memNode*)((char*)cursor+cursor->size+sizeof(memNode));
				continue;
			}

			//original size that was free
			int originalSize=cursor->size;

			//If there is room for the allocation plus a memNode with a size of 1
			if(originalSize>=size+sizeof(memNode)+1) {

				//Update the memNode
				cursor->size=size;
				cursor->inUse=1;

				//Create new memode for the remaining free space
				memNode* newMemNodePtr=(memNode*)((char*)cursor+sizeof(memNode)+size);
				newMemNodePtr->size=originalSize-size-sizeof(memNode);
				newMemNodePtr->inUse=0;

				//Jump back to start of page
				memNode* ptr=(memNode*)pageSpaceStart+pageNodePtr->offset;

				//loop through page and find the largest free region and update the pageNode
				int largestFreeMemory=0;
				while(ptr<(memNode*)endOfPage) {
					if(ptr->inUse==0) {
						if(ptr->size>largestFreeMemory) {
							largestFreeMemory=ptr->size;
						}
					}
				}

				pageNodePtr->largestFreeMemory=largestFreeMemory;

				//return the address of the allocation
				return (void*)(cursor+1);
			}

			//Only room for the allocation
			else if(originalSize>=size){

				//Update the memNode
				cursor->inUse=1;
				cursor->size=size;

				//Jump back to start of page
				memNode* ptr=(memNode*)pageSpaceStart+pageNodePtr->offset;

				//loop through page and find the largest free region and update the pageNode
				int largestFreeMemory=0;
				while(ptr<(memNode*)endOfPage) {
					if(ptr->inUse==0) {
						if(ptr->size>largestFreeMemory) {
							largestFreeMemory=ptr->size;
						}
					}
				}

				pageNodePtr->largestFreeMemory=largestFreeMemory;

				//return the adress of the allocation
				return (void*) (cursor+1);
			}

			//jump to the next memNode
			cursor=(memNode*)((char*)cursor+sizeof(memNode)+cursor->size);
		}

		//Return null if no room in page. This should never happen
		return NULL;

}


void main() {
	int j=7;

	myallocate(10, __FILE__, __LINE__, 0);

}
