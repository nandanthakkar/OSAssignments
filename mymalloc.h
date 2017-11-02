#include <stdbool.h>

#define malloc( x ) mymalloc( x, __FILE__, __LINE__ )

#define free( x ) myfree( x, __FILE__, __LINE__ )

void * mymalloc(size_t size, char* file, int line);

void myfree(void * x, char * file, int line);

typedef struct NodeStruct {
	short size;
	char state;
} Node;


void intializeMyBlock ();

static char * myblock;

static bool myblockIntialized;

