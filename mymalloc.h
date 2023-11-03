#include <stdlib.h>
#include <stdio.h>
#define true 1
#define false 0
#define ptrAddr(ptr) ((unsigned int)(size_t)ptr)

#define nodeSize sizeof(struct Node)



struct Node{
    size_t size;
    struct Node *next;
    struct Node *prev;
};


unsigned int align(unsigned int x);
size_t align_size(size_t x);

struct Node* bestFitSearch(size_t size);
struct Node* firstFitSearch(size_t size);


void myinit(int allocAlg);
void* mymalloc(size_t size);
void myfree(void* ptr);
void* myrealloc(void* ptr, size_t size);
void mycleanup();

