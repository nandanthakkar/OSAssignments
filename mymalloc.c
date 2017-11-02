#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>



/*Defines the Node struct which will hold metadata. The short size
indicates the size of the user data that the Node manages. The char state
is 0 if the memory is not being used. It is 1 if it is being used
*/
typedef struct NodeStruct {
	short size;
	char state;
} Node;

//intializes the char array to be used to simulate dynamic memory
static char myblock[5000];

//boolean to determine whether myblock has been intialized
static bool myblockIntialized=false;

//Initializes the memory block
void intializeMyBlock () {

	//Pointer to the start of the block
	Node * head=(Node *)myblock;

	//Create a node for all the unused space
	Node metadataHead={(5000-(short)sizeof(Node)),0};
	*head=metadataHead;
}

void * mymalloc(size_t size, char* file, int line) {

	/*If the size is > 5000-(the size of a Node), return NULL and
	print an informative error message
	*/ 
	if (size>5000-sizeof(Node))
	{
		printf("Error requesting memory in %s, line %d: The size requested is too large.\n",file ,line);
		return NULL;
	}

	//Initialize memory block if not already done
	if (myblockIntialized==false) {
		intializeMyBlock();
		myblockIntialized=true;
	}

	//Pointer to start of memory block
	Node * ptr=(Node *)myblock;

	/*Loop end if ptr exits the the 5000 bye memory block
	*/ 
	while(((char*)ptr-myblock)<5000) {

		//If the current block is in use, go to the next block
		if((*ptr).state==1) {
			ptr=(Node *)((char*)ptr+sizeof(Node)+(*ptr).size);
			continue;
		}

		short originalSize=(*ptr).size;

		/*If there is room to allocate memory of the inputted size and create
		a new metadata node with a size of at least 1
		*/
		if(originalSize>=(size+(short)sizeof(Node)+1)) {

			/*Update the size of ptr and set it as in use
			*/
			(*ptr).size=size;
			(*ptr).state=1;

			/*Create a new metadata node after ptr, set its size
			as the amount of memory remaming from the original block,
			and set its state to 0
			*/

			Node * next=(Node *)((char*)ptr+size+sizeof(Node));
			(*next).size=originalSize-size-(short)sizeof(Node);
			(*next).state=0;

			//Return ptr
			return (void *) (ptr+1);
		} 

		/*If there is only enough room in the current block to allocate the inputted size,
		allocate all of the block and return ptr
		*/
		else if(originalSize>=size){
			(*ptr).state=1;
			return (void *) (ptr+1);
		}
			
		
		ptr=(Node *)((char*)ptr+sizeof(Node)+(*ptr).size);
	}

		//If ptr exited the memory block then print an error message and return NULL
		printf("Error requesting memory in %s, line %d: Not enough memory available.\n",file ,line);
		return NULL;

}

/* This is a helper method to determine whether a pointer is within the heap.
 * It uses a for loop to traverse through the simulated memory and checks if
 * the pointer is equal to that entry. If it is the function returns '1'. If
 * not the function returns '0'
*/
int isValidEntry(Node * ptr){
	int isValid = 0;

	int i;
	for(i = 0; i < (5000 ); i++) {
		char * head=(char *)myblock;
		if((char*)ptr == (head+i)) {
			isValid = 1;
			break;
		}
	}
	return isValid;
}

void myfree(void * x, char * file, int line) {
	//If p is null, an error message is printed since it is not possile to free a null pointer
	if(x == NULL) {
		printf("Error freeing memory in %s, line %d: Cannot free null pointer\n", file, line);
		return;
	}

	//Creates a pointer of type 'Node'
	Node * ptr = (Node *)((char *)x - sizeof(Node));


	//Checks to see if the pointer is within the heap
	if(isValidEntry(ptr) == 0) {
		printf("Error freeing memory in %s, line %d: Pointer is not in the heap\n", file, line);
		return;
	}

	//Checks if the pointer is already freed
	if(ptr->state == 0) {
		printf("Error freeing memory in %s, line %d: Pointer has no memory allocated\n", file, line);
		return;	
	}
	//Free ptr
	ptr->state=0;

	//Get the metadata after ptr
	Node * next= (Node*)((char*)ptr+sizeof(Node) + ptr->size);

	int nextValidEntry=isValidEntry(next);
	
	//If the next pointer is valid and also not in use, combine
	if(nextValidEntry==1&&next->state==0) {
		ptr->size=ptr->size + sizeof(Node) + next->size;
	}

	Node * curr=(Node*) myblock;
	Node * prev;

	if(curr==ptr) {
		return;
	}

	//Find the metadata before ptr
	while(curr!=ptr) {
		prev=curr;
		curr=(Node*)((char*)curr + sizeof(Node) + curr->size); 
	}

	//If the metadata prev pointer is free, combine
	if(prev->state==0) {
		prev->size=prev->size+sizeof(Node)+ptr->size;
	}

}

