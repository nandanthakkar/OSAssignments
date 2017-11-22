#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include "memory.h"

int getCurrentThread();

char getInMemoryCFile() {
	return inMemoryCFile;
}

//error function to print message and exit
void fatalError(int line, char* file) {
	printf("Error:\n");
	printf("ERROR= %i\n", strerror(errno));
	printf("Line number= %i \nFile= %s\n", line, file);
	exit(-1);
}

int protectMemory(){
  	//printf("start protectMemory\n");
	int threadID = getCurrentThread();

	//printf("thread id current %d\n", threadID );
	mprotect(memory,bytesMemory,PROT_READ | PROT_WRITE);
	int maxPage = -1;	
	//temp pageNode to find all pages of given thread
	pageNode * pageNodeAddr = (pageNode *)rightBlockStart;
	//loop to scroll through all page nodes
	while((char *)pageNodeAddr < (char *)(rightBlockStart + OSRightBlockSize)){
    		if(pageNodeAddr->offset > 0 && pageNodeAddr->threadId==threadID){
      			if(maxPage < pageNodeAddr->pageId){
        			maxPage = pageNodeAddr->pageId;
      			}
    		}	
    		pageNodeAddr= pageNodeAddr +1;
  	}
 	/*if(mprotect(memory,OSSize,PROT_NONE) == -1){
		perror("mprotect did not work");
		fatalError(__LINE__,__FILE__);
	}*/
	void * addr = pageSpaceStart + maxPage*pageSize;
	size_t size = (size_t)(((char *)memory + bytesMemory - (pageSize * 4)) - (char *)addr);

	//printf("addr: %lu, sizeof size_t,size: %ld\n",addr,sizeof(size_t),size);
	if(size > 0){
		int i =	mprotect(addr, size,PROT_NONE);	
		if(i == -1){
			perror("mprotect did not work");
			fatalError(__LINE__,__FILE__);
		}
	}
	//printf("Return from protectMemory\n");
}

//function to swap pages from memory to file
//swap function that swaps pages
int swap(pageNode* inSwap) {
	int fd=open("swapFile.txt", O_RDWR);
	//printf("Start swap\n");
	pageNode* inMem=NULL;
	pageNode* cursor=NULL;

	//index of the memory location used as a temp location to swap
	int tempIndex=bytesMemory - (pageSize * 7);

	//find node in swap
	cursor=(pageNode*)rightBlockStart;
	while((int*)cursor < (int*)pageSpaceStart){
		//printf("Loop1\n");
		if(cursor->offset == (inSwap->pageId-1) * pageSize + 1) {
			inMem=cursor;
			break;
		}

		cursor=cursor+1;
	}

	//copy memory page into temp memory page
	memcpy(memory + tempIndex, pageSpaceStart + inMem->offset - 1, pageSize);

	//read from swap into to Memory
	lseek(fd, -1+(-1)*inSwap->offset, SEEK_SET);//changed to SEEK_SET to access data from offset + 0. NT
	int readAmount=read(fd, pageSpaceStart + inMem->offset -1, pageSize);//Added -1 to offset: NT
	while(readAmount<pageSize) {
		//printf("in loop: %d\n", readAmount);
		readAmount+=read(fd, pageSpaceStart + inMem->offset -1+readAmount, pageSize-readAmount);
	}
	lseek(fd,-1*pageSize,SEEK_CUR);
	//lseek(fd, SEEK_CUR - 1, 0);  Not sure if that is needed. NT

	//write temp into swap
	int writeAmount=write(fd, memory + tempIndex, pageSize);

	while(writeAmount<pageSize) {
		writeAmount+=write(fd, memory + tempIndex + writeAmount, pageSize-writeAmount);
	}
	//lseek(fd, SEEK_CUR - 1, 0); Not sure if this line needed. NT

	//update Offsets
	//remember inMem is now in swap and inSwap is now in mem
	//int tempOffset = inMem->offset; // added temp to save offset
	inMem->offset=inSwap->offset; // put inSwap offset inside inMem offset. NT
	inSwap->offset=(inSwap->pageId - 1) * (pageSize) + 1;
	close(fd);
	//printf("Return from swap\n");
  return 1;
}

int memAlignPages(){
	int threadID = getCurrentThread();

	//printf("thread id current %d\n", threadID );
	mprotect(memory,bytesMemory,PROT_READ | PROT_WRITE);
	int maxPage = -1;	
	//temp pageNode to find all pages of given thread
	pageNode * pageNodeAddr = (pageNode *)rightBlockStart;
	//loop to scroll through all page nodes
	while((char *)pageNodeAddr < (char *)(rightBlockStart + OSRightBlockSize)){

		if(pageNodeAddr->threadId==threadID) {
			if(pageNodeAddr->pageId > maxPage){
				maxPage = pageNodeAddr->pageId;
			}
		}

		// if found page of a thread then align it.
		if(pageNodeAddr->threadId == threadID && pageNodeAddr->offset != (((pageNodeAddr->pageId)-1)*pageSize)+1){
			//printf("if fired\n");
			//if page node we  need to align is in memory
			if(pageNodeAddr->offset > 0){
				
				//create temp page node to find if space where we want align if free or occupied
				pageNode * pageTemp = (pageNode *)rightBlockStart;
				
				// created to find if space where we want to align  is occupied.
				char isOccupied = 0;
				
				//find page node for space where we want to align
				while((char *)pageTemp < (char *)(rightBlockStart + OSRightBlockSize)){
					//printf("Loop3\n");
					
					// if found a
					if(pageTemp->offset == (((pageNodeAddr->pageId)-1) * pageSize)+1){
						memcpy((void*)((char *)memory + bytesMemory - pageSize*7), (void*)((char *)pageSpaceStart + (pageTemp->offset) -1), pageSize);
						memcpy((void*)((char *)pageSpaceStart + pageTemp->offset -1), (void*)((char *)pageSpaceStart + pageNodeAddr->offset -1), pageSize);
						memcpy((void*)((char *)pageSpaceStart + (pageNodeAddr->offset) - 1),(void*)((char *)memory + bytesMemory - pageSize*7), pageSize);
						memset((void*)((char *)memory + bytesMemory - pageSize*7), 0, pageSize);
						int offsetTemp = pageTemp->offset;
						pageTemp->offset = pageNodeAddr->offset;
						pageNodeAddr->offset = offsetTemp;
						isOccupied = 1;
					}
					
					pageTemp++;
				}
				
				if(!isOccupied){
					//printf("Current thread: %d",getCurrentThread());
					//printf("ThreadId: %d, pageId: %d, offset: %d", pageNodeAddr->threadId, pageNodeAddr->pageId, pageNodeAddr->offset);
					perror("did not find pageNode for location to swap to");
					fatalError(__LINE__,__FILE__);
					// memcpy((void*)((char *)pageSpaceStart + (pageNodeAddr->pageId - 1) * pageSize),(void*)(memory + pageNodeAddr->offset-1));
					// pagNodeAddr->offset = (int)((pageNodeAddr->pageId - 1) * pageSize) + 1 ;
				}
			}
			
			if(pageNodeAddr->offset < 0){
				int i = swap(pageNodeAddr);
				if(i == 0){
					perror("swap function did not work");
					fatalError(__LINE__,__FILE__);
				}
			}
		}

		pageNodeAddr=pageNodeAddr+1;

	}
	/*if(mprotect(memory,OSSize,PROT_NONE) == -1){
		perror("mprotect did not work");
		fatalError(__LINE__,__FILE__);
	}*/
	void * addr = pageSpaceStart + maxPage*pageSize;
	size_t size = (size_t)(((char *)memory + bytesMemory - (pageSize * 4)) - (char *)addr);

	//printf("addr: %lu, size: %d\n",addr,size);
	if(size > 0){
		int i =	mprotect(addr, size,PROT_NONE);
		if(i == -1){
			perror("mprotect did not work");
			fatalError(__LINE__,__FILE__);
		}
	}
	//printf("Return from memalign\n");
	return 1;
}
void* allocateMemoryInPage(size_t size, pageNode* pageNodePtr);

//NOTE: argument 2 changed to char* from int -Steve 11-9-2017 9:23AM
void* myallocate(size_t size, char* file1, int line1, int thread) {
	inMemoryCFile=1;

	mprotect(memory,bytesMemory,PROT_READ | PROT_WRITE);

	//printf("Start my allocate\n");
	int x=-1;
	//first time malloc is called, need to initalize mmory array
	if(firstTimeMalloc==1) {
		//printf("First time malloc\n");
		//open file
		/*int fd=open("swapFile.txt", O_RDWR | O_CREAT | O_TRUNC, 0666);
		if(fd == -1) {
			fatalError(__LINE__, __FILE__);
		}

		//lseek to make file 100 bytes and then set back to beinging
		x=lseek(fd, totalBytes - bytesMemory-1, SEEK_CUR);
		if(x != totalBytes - bytesMemory-1){
			fatalError(__LINE__, __FILE__);
		}

		x=lseek(fd,0,SEEK_SET);
		*/

		FILE* fp=fopen("swapFile.txt","w");
		ftruncate(fileno(fp),totalBytes-bytesMemory);

		fclose(fp);

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

		pageNode* pageNodeAddr=currentPageNode;
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

		//printf("memory nodes created\n");

		int numNodesInMemory=pageNodeCreatedCounter;

		//Initialize the pageNodes of pages not in memory
		while(pageNodeCreatedCounter<totalPagesMemoryAndSwapFile) {
			pageNode newPageNode={-2,0,0,-1 - (pageNodeCreatedCounter-numNodesInMemory)*pageSize};
			*currentPageNode=newPageNode;

			currentPageNode+=1;
			pageNodeCreatedCounter++;
		}

		memNode* firstSharedPage=(memNode*)(pageSpaceStart+(totalPagesUser + alwaysFreePages)*pageSize);

		firstSharedPage->inUse=0;
		firstSharedPage->size=(pageSize*4-sizeof(memNode));

		//printf("Swap nodes created\n");

	}

	//Library called malloc
	if(thread==0) {

		//The start of the os left block
		memNode* cursor=(memNode*)memory;

		//Loop through the entire left block
		while(cursor<(memNode*)(memory+OSLeftBlockSize)) {
			//printf("Loop6\n");

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

				//Create a new memNode for remaining free space
				memNode* newMemNodePtr=(memNode*)((char*)cursor+sizeof(memNode)+size);
				newMemNodePtr->size=originalSize-size-sizeof(memNode);
				newMemNodePtr->inUse=0;

				//printf("TEST1\n");

				protectMemory();

				//return the adress of the allocation
				inMemoryCFile=0;
				return (void*)(cursor+1);
			}

			//If there is only room for the allocation update the memNode to refer to the new allocation and return the adress
			//of the new allocation
			else if(originalSize>=size){
				cursor->inUse=1;
				//printf("TEST2\n");
				protectMemory();
				inMemoryCFile=0;
				return (void *) (cursor+1);
			}

			//jump to the next memNode
			cursor=(memNode*)((char*)cursor+sizeof(memNode)+cursor->size);
		}

		//If no space available return NULL
		protectMemory();
		inMemoryCFile=0;
		return NULL;

	}

	//User made call to malloc
	else {

		//printf("current thread id in malloc: %d\n",getCurrentThread() );

		//Start of pageNodes
		pageNode* currentPageNode=(pageNode*)rightBlockStart;

		//Count of pageNodes seen with corresponding threadId
		int pageCount=0;

		//Count of all pages seen
		int pageCounter=0;

		//If the allocation can fit inside one page
		if(size<=freshPageFreeSize) {

			//printf("allocation can fit inside: %d\n",size );

			//Loop through all pageNodes
			while(pageCounter<totalPagesMemoryAndSwapFile - alwaysFreePages - sharedRegionPageNumber) {
				//printf("Loop7\n");

				//If you find a pageNode with correspoding threadId
				if(currentPageNode->threadId==getCurrentThread()) {


					//In memory and there is space available in the page
					if(currentPageNode->offset>0&&currentPageNode->largestFreeMemory>=size) {
						//printf("TEST\n");

						void* output=allocateMemoryInPage(size,currentPageNode);

						//printf("output %p\n",(void*)output);

						//allocate the memory
						protectMemory();
						inMemoryCFile=0;
						return (void*)output;
					}

					//In swap file and there is space avaible
					else if (currentPageNode->offset<0&&currentPageNode->largestFreeMemory>=size) {
						//printf("TEST2\n");
						//bring page from swap file into memory

						//allocte the memory
						void* output=allocateMemoryInPage(size,currentPageNode);

						//printf("output %p\n",(void*)output);

						//allocate the memory
						protectMemory();
						inMemoryCFile=0;
						return (void*)output;
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
				//printf("returning null pagecount>=totalPagesUser\n");
				protectMemory();
				inMemoryCFile=0;
				return NULL;
			}

			//Re-initialize these variable since loop starts from beginning again
			pageCounter=0;
			currentPageNode=(pageNode*)rightBlockStart;

			//Loop through all pageNodes
			while(pageCounter<totalPagesMemoryAndSwapFile - sharedRegionPageNumber - alwaysFreePages) {
				//printf("Loop8\n");

				//pageNode is free
				if(currentPageNode->threadId==0) {

					//Set its thread, pageId, and largestFreeMemory
					currentPageNode->threadId=getCurrentThread();
					currentPageNode->pageId=pageCount+1;
					currentPageNode->largestFreeMemory=freshPageFreeSize;

					//If the pageNode refers to a page not in memory than bring it into memory and initialize the page
					if(currentPageNode->offset<0) {
						swap(currentPageNode);
						memNode* startOfPage=(memNode*)(pageSpaceStart+currentPageNode->offset-1);
						startOfPage->inUse=0;
						startOfPage->size=freshPageFreeSize;
					}

					//printf("TEST3\n");

					memAlignPages();
					//allocate the memory
					void* output= allocateMemoryInPage(size, currentPageNode);
					protectMemory();
					inMemoryCFile=0;
					return output;
				}

				pageCounter++;
				currentPageNode+=1;

			}

			//printf("returning null all pages used\n");
			//If all pageNodes used, no more memory to allocate
			protectMemory();
			inMemoryCFile=0;
			return NULL;
		}

		//Request will span two or more pages
		else {
			//printf("allocation cannot fit inside: %d\n",size );

			//Last Page Node allocated to the thread
			pageNode* highestPageId=NULL;

			//Find the highest pageId allocated to the thread
			while(currentPageNode<(pageNode*)(rightBlockStart+OSRightBlockSize-7*sizeof(pageNode))) {
				if(currentPageNode->threadId==getCurrentThread()) {
					if(highestPageId==NULL||currentPageNode->pageId>highestPageId->threadId) {
						highestPageId=currentPageNode;
					}
				}

				currentPageNode+=1;
			}

			//printf("done with loop 9\n");



			//No page has been allocated to the thread
			if(highestPageId==NULL) {

				//The number of pages thar are going to have to be allocated for the request
				int newPagesNum=(size+sizeof(memNode))/pageSize;

				if((size+sizeof(memNode))%pageSize!=0) {
					newPagesNum++;
				}

				//If the total request won't be able to fit into memory at one time return null
				if(newPagesNum>totalPagesUser) {
					//printf("Returning null newPagesNum>totalPagesUser\n");
					protectMemory();
					inMemoryCFile=0;
					return NULL;
				}

				//Array of pageNode* to hold the free pageNodes that are found
				pageNode* freeNodes[newPagesNum];

				//Current page node
				pageNode* currentPageNode=(pageNode*)rightBlockStart;

				//index for the array of pageNode*
				int index=0;

				//loop through all the pageNodes
				while(currentPageNode<(pageNode*)(rightBlockStart+OSRightBlockSize-7*sizeof(pageNode))) {
					//printf("Loop10\n");

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
				}

				//Not enough memory left for request return null
				if(index<newPagesNum) {
					//printf("Returning index<newPagesNum\n");
					protectMemory();
					inMemoryCFile=0;
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
					swap(firstPageNode);
				}

				memAlignPages();

				//Start of the page in memory
				memNode* startOfPage=pageSpaceStart;

				//printf("page space start %lu\n",pageSpaceStart);

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
						swap(pgNode);
					}
				}

				//Return the address of the first page in memory
				void* output= pageSpaceStart+sizeof(memNode);
				protectMemory();
				inMemoryCFile=0;
				return output;


			}

			else {
				int newPagesNum=(size+sizeof(memNode))/pageSize;

				if((size+sizeof(memNode))%pageSize!=0) {
					newPagesNum++;
				}

				if(newPagesNum+highestPageId->pageId>totalPagesUser) {
					//printf("Returning null newPagesNum+highestPageId->pageId>totalPagesUser\n");
					protectMemory();
					inMemoryCFile=0;
					return NULL;
				}


				//Array of pageNode* to hold the free pageNodes that are found
				pageNode* freeNodes[newPagesNum];


				//Current page node
				pageNode* currentPageNode=(pageNode*)rightBlockStart;

				//index for the array of pageNode*
				int index=0;

				//loop through all the pageNodes
				while(currentPageNode<(pageNode*)(rightBlockStart+OSRightBlockSize-7*sizeof(pageNode))) {
					//printf("Loop11\n");

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
				}

				//Not enough memory left for request reurn null
				if(index<newPagesNum) {
					//printf("Returning null index<newPagesNum\n");
					protectMemory();
					inMemoryCFile=0;
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
					swap(firstPageNode);
				}

				memAlignPages();

				//Start of the page in memory
				memNode* startOfPage=(memNode*)((char*)pageSpaceStart+firstPageNode->offset-1);

				//printf("start of page %lu\n",startOfPage );

				//Initialize the memNode
				startOfPage->inUse=1;
				startOfPage->size=size;

				int i=1;

				//Initialize  the rest of the memNodes and bring them into memory if necessary
				for(i;i<newPagesNum;i++) {
					pageNode* pgNode=freeNodes[i];
					pgNode->threadId=getCurrentThread();
					pgNode->largestFreeMemory=-1;
					pgNode->pageId=highestPageId->pageId+i+1;
					if(pgNode->offset<0) {
						swap(pgNode);
					}
				}


				void* output= (void*)(startOfPage + 1);

				protectMemory();
				inMemoryCFile=0;
				return output;

			}

		}

	}

	protectMemory();
	inMemoryCFile=0;
}

/*
	Function to service a malloc request after the correct page has already been identified
*/
void* allocateMemoryInPage(size_t size, pageNode* pageNodePtr) {

	//printf("start allocateMemoryInPage: %lu\n",pageNodePtr);

	//location of beginning of page
	memNode* cursor=(memNode*)((char*)pageSpaceStart+pageNodePtr->offset-1);

	//location of end of page
	void* endOfPage=(void*)cursor+pageSize;

	//printf("edn of pahe: %lu\n",endOfPage);


	while(cursor<(memNode*)endOfPage) {
		//printf("Loop12\n");

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

				//Create new memNode for the remaining free space
				memNode* newMemNodePtr=(memNode*)((char*)cursor+sizeof(memNode)+size);
				newMemNodePtr->size=originalSize-size-sizeof(memNode);
				newMemNodePtr->inUse=0;

				//Jump back to start of page
				memNode* ptr=(memNode*)((char*)pageSpaceStart+pageNodePtr->offset-1);

				//loop through page and find the largest free region and update the pageNode
				int largestFreeMemory=0;
				while(ptr<(memNode*)endOfPage) {
					//printf("Loop13\n");	
					if(ptr->inUse==0) {
						if(ptr->size>largestFreeMemory) {
							largestFreeMemory=ptr->size;
						}
					}
	
					ptr=(memNode*)((char*)ptr+sizeof(memNode)+ptr->size);
				}

				pageNodePtr->largestFreeMemory=largestFreeMemory;


				//printf("my allocate return: %u\n",(cursor+1));

				//return the address of the allocation
				return (void*)(cursor+1);
			}

			//Only room for the allocation
			else if(originalSize>=size){

				//Update the memNode
				cursor->inUse=1;
				cursor->size=originalSize;

				//Jump back to start of page
				memNode* ptr=(memNode*)((char*)pageSpaceStart+pageNodePtr->offset-1);

				//loop through page and find the largest free region and update the pageNode
				int largestFreeMemory=0;
				while(ptr<(memNode*)endOfPage) {
					if(ptr->inUse==0) {
						if(ptr->size>largestFreeMemory) {
							largestFreeMemory=ptr->size;
						}
					}
					ptr=(memNode*)((char*)ptr+sizeof(memNode)+ptr->size);
					//printf("Loop14\n");
				}

				pageNodePtr->largestFreeMemory=largestFreeMemory;

				//return the adress of the allocation
				return (void*) (cursor+1);
			}

			//jump to the next memNode
			cursor=(memNode*)((char*)cursor+sizeof(memNode)+cursor->size);
		}

		//printf("Return from allocateMemoryInPage null\n");

		//Return null if no room in page. This should never happen
		return NULL;

}

void* shalloc(size_t size) {
	inMemoryCFile=1;

		//printf("shalloc started\n");


		if(firstTimeMalloc==1) {
			int x=-1;
			//printf("First time malloc\n");
			//open file
			/*int fd=open("swapFile.txt", O_RDWR | O_CREAT | O_TRUNC, 0666);
			if(fd == -1) {
				fatalError(__LINE__, __FILE__);
			}

			//lseek to make file 100 bytes and then set back to beinging
			x=lseek(fd, totalBytes - bytesMemory-1, SEEK_CUR);
			if(x != totalBytes - bytesMemory-1){
				fatalError(__LINE__, __FILE__);
			}

			x=lseek(fd, SEEK_CUR - 1, 0);
			if(x != 0) {
				fatalError(__LINE__, __FILE__);
			}*/
	        	/*sa.sa_flags = SA_SIGINFO;
	        	sigemptyset(&sa.sa_mask);
	        	sa.sa_sigaction = memoryhandler;

	       	 	if (sigaction(SIGSEGV, &sa, NULL) == -1)
	       		{
	        	    printf("Fatal error setting up signal handler\n");
	        	    exit(EXIT_FAILURE);    //explode!
		        }*/

			FILE* fp=fopen("swapFile.txt","w");
			ftruncate(fileno(fp),totalBytes-bytesMemory);
			fclose(fp);

			//Macro to get the page size
			pageSize=sysconf(_SC_PAGE_SIZE);

			//close(fd);

			x=posix_memalign(&memory,pageSize,bytesMemory);
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

			pageNode* pageNodeAddr=currentPageNode;
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

			//printf("memory nodes created\n");

			int numNodesInMemory=pageNodeCreatedCounter;

			//Initialize the pageNodes of pages not in memory
			while(pageNodeCreatedCounter<totalPagesMemoryAndSwapFile) {
				pageNode newPageNode={-2,0,0,-1 - (pageNodeCreatedCounter-numNodesInMemory)*pageSize};
				*currentPageNode=newPageNode;

				currentPageNode+=1;
				pageNodeCreatedCounter++;
			}

			memNode* firstSharedPage=(memNode*)(pageSpaceStart+(totalPagesUser + alwaysFreePages)*pageSize);

			firstSharedPage->inUse=0;
			firstSharedPage->size=(pageSize*4-sizeof(memNode));

			//printf("Swap nodes created\n");
		}


		//The start of the shared space
		memNode* cursor=(memNode*)(memory+(totalPagesInMemory-4)*pageSize);

		//printf("addrss of cursor shalloc: %lu\n",cursor);

		//Loop through the entire shared block
		while(cursor<(memNode*)(memory+totalBytes)) {
			//printf("Loop15\n");

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

				//Create a new memNode for remaining free space
				memNode* newMemNodePtr=(memNode*)((char*)cursor+sizeof(memNode)+size);
				newMemNodePtr->size=originalSize-size-sizeof(memNode);
				newMemNodePtr->inUse=0;

				//return the adress of the allocation
				inMemoryCFile=0;
				return (void*)(cursor+1);
			}

			//If there is only room for the allocation update the memNode to refer to the new allocation and return the adress
			//of the new allocation
			else if(originalSize>=size){
				cursor->inUse=1;
				inMemoryCFile=0;
				return (void *) (cursor+1);
			}

			//jump to the next memNode
			cursor=(memNode*)((char*)cursor+sizeof(memNode)+cursor->size);
		}

		//If no space available return NULL
		inMemoryCFile=0;
		return NULL;
}

void mydeallocate(void* address, char* file1, int line1, int thread) {
	inMemoryCFile=1;
	//printf("Start my deallocate on : %lu\n",address);
	mprotect(memory,bytesMemory,PROT_READ | PROT_WRITE);
	if(address == NULL) {
		//printf("Error freeing memory in %s, line %d: Cannot free null pointer\n", file1, line1);
		protectMemory();
		inMemoryCFile=0;
		return;
	}

	//freeing of address before array
	if(address<memory){
		//printf("Error freeing memory in %s, line %d: Address outside of memory block\n", file1, line1);
		protectMemory();
		inMemoryCFile=0;
		return;
	}

	//freeing of ptr in OS Left Block
	else if(address<rightBlockStart) {

		//Creates a pointer of type memNode
		memNode* ptr = (memNode*)(address - sizeof(memNode));

		//printf("Size: %d\n", ptr->size);

		//printf("Size plus ptr: %lu\n",ptr->size+address );

		//Checks if the pointer is already freed
		if(ptr->inUse == 0) {
			//printf("Error freeing memory in %s, line %d: Pointer has no memory allocated\n", file1, line1);
			protectMemory();
			inMemoryCFile=0;
			return;
		}

		//Free ptr
		ptr->inUse=0;

		//Get the metadata after ptr
		memNode* next= (memNode*)((char*)ptr+sizeof(memNode) + ptr->size);

		if(next<(memNode*)rightBlockStart&&next->inUse==0) {
			ptr->size=ptr->size + sizeof(memNode) + next->size;
		}

		memNode* curr=(memNode*) memory;
		memNode* prev=NULL;

		if(curr==ptr) {
			protectMemory();
			inMemoryCFile=0;
			return;
		}

		//Find the metadata before ptr
		while(curr!=ptr) {
			//printf("Loop16\n");
			prev=curr;
			curr=(memNode*)((char*)curr + sizeof(memNode) + curr->size);
		}

		//If the metadata prev pointer is free, combine
		if(prev->inUse==0) {
			prev->size=prev->size+sizeof(memNode)+ptr->size;
		}

	}

	//freeing of PageNode
	else if (address<pageSpaceStart) {
		//printf("Error freeing memory in %s, line %d: Dont free pageNodes\n", file1, line1);
		protectMemory();
		inMemoryCFile=0;
		return;
	}

	//freeing of address inside page space region
	else if (address<pageSpaceStart+pageSize*totalPagesUser) {

		//Creates a pointer of type memNode
		memNode* ptr = (memNode*)(address - sizeof(memNode));

		memNode* pageStart=(memNode*)((unsigned long)address-(unsigned long)address%pageSize);

		memNode* pageEnd=(memNode*)((char*)pageStart+pageSize);


		//printf("ptr: %lu, pagestart %lu, pageEnd %lu\n",ptr,pageStart,pageEnd );

		//Checks if the pointer is already freed
		if(ptr->inUse == 0) {
			//printf("Error freeing memory in %s, line %d: Pointer has no memory allocated\n", file1, line1);
			protectMemory();
			inMemoryCFile=0;
			return;
		}

		//Free ptr
		ptr->inUse=0;

		//Get the metadata after ptr
		memNode* next= (memNode*)((char*)ptr+sizeof(memNode) + ptr->size);

		if(next<pageEnd&&next->inUse==0) {
			ptr->size=ptr->size + sizeof(memNode) + next->size;
		}

		memNode* curr=pageStart;
		memNode* prev=NULL;

		if(curr==ptr) {
			protectMemory();
			inMemoryCFile=0;
			return;
		}

		//Find the metadata before ptr
		while(curr!=ptr) {
			//printf("Loop17: %lu\n curr->size: %lu\n",curr,curr->size);
			prev=curr;
			curr=(memNode*)((char*)curr + sizeof(memNode) + curr->size);
		}

		//If the metadata prev pointer is free, combine
		if(prev->inUse==0) {
			prev->size=prev->size+sizeof(memNode)+ptr->size;
		}


		//update the pageNode

		int offsetShouldBe=(unsigned long)address - (unsigned long) address%pageSize + 1;

		int largestFreeMemory=0;

		pageNode* currentPageNode=(pageNode*)rightBlockStart;

		while(currentPageNode<(pageNode*)(rightBlockStart+OSRightBlockSize-7*sizeof(pageNode))) {
			if(currentPageNode->offset==offsetShouldBe) {
				memNode* mNode=(memNode*)(pageSpaceStart+currentPageNode->offset-1);
				while(mNode<(memNode*)(pageSpaceStart + currentPageNode->offset -1 + pageSize)) {
					if(mNode->inUse==0 && mNode->size>largestFreeMemory) {
						largestFreeMemory=mNode->size;
					}

					mNode=(memNode*)((char*)mNode+sizeof(memNode)+mNode->size);
				}

				currentPageNode->largestFreeMemory=largestFreeMemory;
				break;
			}

			currentPageNode+=1;
		}


	}

	else if(address<pageSpaceStart+pageSize*totalPagesUser+pageSize*alwaysFreePages) {
		//printf("Error freeing memory in %s, line %d: Dont free in always free pages\n", file1, line1);
		protectMemory();
		inMemoryCFile=0;
		return;
	}

	//freeing inside shared region
	else if(address<memory+pageSize*totalPagesInMemory) {

		//Creates a pointer of type memNode
		memNode* ptr = (memNode*)(address - sizeof(memNode));

		//Checks if the pointer is already freed
		if(ptr->inUse == 0) {
			//printf("Error freeing memory in %s, line %d: Pointer has no memory allocated\n", file1, line1);
			protectMemory();
			inMemoryCFile=0;
			return;
		}

		//Free ptr
		ptr->inUse=0;

		//Get the metadata after ptr
		memNode* next= (memNode*)((char*)ptr+sizeof(memNode) + ptr->size);

		if(next<(memNode*)(memory+pageSize*totalPagesInMemory)&&next->inUse==0) {
			ptr->size=ptr->size + sizeof(memNode) + next->size;
		}

		memNode* curr=(memNode*)(pageSpaceStart+pageSize*totalPagesUser+pageSize*alwaysFreePages);
		memNode* prev=NULL;

		if(curr==ptr) {
			protectMemory();
			inMemoryCFile=0;
			return;
		}

		//Find the metadata before ptr
		while(curr!=ptr) {
			//printf("Loop18\n");
			prev=curr;
			curr=(memNode*)((char*)curr + sizeof(memNode) + curr->size);
		}

		//If the metadata prev pointer is free, combine
		if(prev->inUse==0) {
			prev->size=prev->size+sizeof(memNode)+ptr->size;
		}
	}
	protectMemory();
	inMemoryCFile=0;
	//printf("Return from my deallocate\n");


}

/*	Function to clean out pages of thread that is completing. Should be called in thread complete
*/
void cleanOutPageNodes(int threadId) {
	inMemoryCFile=1;
	mprotect(memory,bytesMemory,PROT_READ | PROT_WRITE);
	pageNode* currentPageNode=(pageNode*)rightBlockStart;
	while(currentPageNode<(pageNode*)(rightBlockStart+OSRightBlockSize-7*sizeof(pageNode))) {
		if(currentPageNode->threadId==threadId) {
			if(currentPageNode->offset<0) {
				swap(currentPageNode);
			}
			memNode* firstMemNode=(memNode*)(pageSpaceStart+currentPageNode->offset-1);
			firstMemNode->inUse=0;
			firstMemNode->size=freshPageFreeSize;

			currentPageNode->largestFreeMemory=freshPageFreeSize;
			currentPageNode->threadId=0;
			currentPageNode->pageId=0;
		}

		currentPageNode+=1;
	}

	protectMemory();
	inMemoryCFile=0;
}
