#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <assert.h>
#include "mymalloc.h"

#define MAX_HEAP (1024*1024)
/*
 * Globals that will be maintained throughout the program
 * but initialized in init and reset in cleanup
 */
enum allocAlgos{first_f, next_f, best_f};
enum allocAlgos allocAlgo;
char* heap;
unsigned int heapAddress = 0;
struct Node* head;

struct Node* tail;
/*
 * Init will call our heap into memory, as well as initialize our globals
 */
void myinit(int allocAlg){
    //set allocAlgo
    allocAlgo = allocAlg;
    //set heap to char* size MAX_HEAP -> char* to ensure 1byte arithmetic
    heap = (char *) malloc(MAX_HEAP);
    //set the address of the heap
    heapAddress = (unsigned int)(size_t)heap;
    //initialize head and tail
    head = (void*)malloc(nodeSize);
    tail = (void*)malloc(nodeSize);

    struct Node* temp = (void*)heap;
    

    head->prev = NULL;

    head->next = temp;
    tail->prev = temp;
    tail->next = NULL;

    temp->prev = head;
    temp->next = tail;
    temp->size = MAX_HEAP;

    size_t * footer = (void*)heap + MAX_HEAP - sizeof(size_t);
    *footer = MAX_HEAP;
    
    printf("\tLocation of head: %p\n", head);
    printf("\tLocation of tail: %p\n", tail);
    printf("\tinit called\n");
}

void mycleanup(){
    free(heap);
    free(head);
    free(tail);
    head = NULL;
    tail = NULL;
    heap = NULL;
    heapAddress = 0;
    printf("\tcleanup called\n");
}

void* mymalloc(size_t size){
    //if size == 0, we just return NULL
    if(size == 0) return NULL;
    //align size to meet 8 byte alignment rule
    size = align_size(size);

    if(size < 16) size = 16;

    //set ptr to be used for allocation
    struct Node* ptr;
    
    //if we use firstFit, then use firstfit search to set ptr; first_f == 0
    if(allocAlgo == first_f){
        ptr = firstFitSearch(size);
    }

    //if nextFit, we don't need to search, just maintain tail next_f == 1
    if(allocAlgo == next_f){
        ptr = tail->prev;
    }

    //if bestfit, use bestfitsearch to set ptr best_f == 2
    if(allocAlgo == best_f){
        ptr = bestFitSearch(size);
    }

    //if any of the algos return tail, then we know that there is no space that the request could
    //fit in, and so we return null
    if(ptr == tail) return NULL;
    //now ptr is a freenode that is <= size of size
    //if freenode > size + nodesize then we will have to shrink the ptr freenode
    if(ptr->size - (size + 2*sizeof(size_t)) > nodeSize + sizeof(size_t)) {
        //we'll be moving ptr so we set a temp variable to maintain the address
        struct Node* temp = ptr;
        //the shrunken size of temp is set
        
        //move temp up size + nodeSize bytes 
        //casts struct Node*->char*->struct Node* so ptr arithmetic can b done
        //in units of ONE byte

        temp = (struct Node*)((char*)temp+(size + 2*sizeof(size_t)));
        temp->size = ptr->size - (size + 2*sizeof(size_t));
        // printf("temp at %p\n", temp);
        // printf("ptr at: %p\n", ptr);
        // printf("ptr->size: %ld\n", ptr->size);
        // printf("temp->size: %lu\n", temp->size);
        // printf("value of temp struct: %lu\n", temp->size);
        
        //set temp's footer as well
        *(size_t*)((char*)temp+(temp->size-sizeof(size_t))) = temp->size;

        //now that the new freenode is set, we can adjust the freelist
        //this involves setting next's and prev's references to ptrs to temp.

        //if no prev, no prev->next to set
        if(ptr->prev != NULL){
            ptr->prev->next = temp;
        }
        //if no next, no next->prev to set
        if(ptr->next != NULL){
            ptr->next->prev = temp;
        }
        //set temp's next and prev to ptr's next and prev
        temp->prev = ptr->prev;
        temp->next = ptr->next;
        if(ptr == head->next) head->next = temp;
        if(ptr == tail->prev) tail->prev = temp;
        //set ptr's next and prev to null now they're no longer needed
        ptr->prev = NULL;
        ptr->next = NULL;

        //set ptr size to new size now
        ptr->size = align_size(size) + 2*sizeof(size_t) + 1;
        //sets footer
        *(size_t*)((char*)ptr+(size+sizeof(size_t))) = align_size(size) + 2*sizeof(size_t) + 1;

    }
    //if freenode's size is equal to the size of the new allocation
    //we simply allocate and then remove links to the ptr from prev and next
    else{
        //if there is no prev, there is no prev->next to set
        if(ptr->prev != NULL){
            ptr->prev->next = ptr->next;
        }
        //if there is no next there is no next->prev to set
        if(ptr->next != NULL){
            ptr->next->prev = ptr->prev;
        }
        //set the pointers references to these to null
    

    }   


    
    //now that pointer and the freelist have been properly handled,
    //we can return a pointer that points to th ebeginning of the payload to
    //the user
    //printf("Allocation %d at address %d\n", (int)size, (int)ptrAddr(ptr+sizeof(size_t))-ptrAddr(heap));
    return (void*)((char*)ptr + sizeof(size_t));


}

void* myrealloc(void* ptr, size_t requested_size) {
    // base cases
    if (ptr == NULL && requested_size == 0)
        return NULL;
    
    if (ptr == NULL)
        return mymalloc(requested_size);

    if (requested_size == 0) {
        myfree(ptr);
        return NULL;
    }

    // header and footer of the block corresponding to ptr
    size_t* header = ptr-sizeof(size_t);
    size_t* footer = (size_t*)((char*)header+(*header & ~1)-sizeof(size_t));

    if (requested_size < *header) {
        return ptr; // instructions say reallocate to AT LEAST the specified size, so changing nothing technically follows that
    }


    // header of the block adjacent to current block
    size_t* next_header = (size_t*)((char*)footer + sizeof(size_t));
    
    // make sure the next footer would not extend past the heap
    size_t* next_footer = (size_t*)((char*)header + align_size(requested_size) + sizeof(size_t));
    if ((char*)next_footer > (heap+MAX_HEAP))
        *next_header = 1; // fail the first check, reallocate somewhere else


    // if the next block is free, and has enough size to accomodate the requested size and still leave space for a node and footer...
    if (*next_header % 2 == 0 && (*next_header == requested_size-*header || *next_header - (requested_size-*header) >= 32)) {
        printf("Extending into the next block\n");
        // printf("size of next block: %lu\n", *next_header);


        size_t* new_footer = (size_t*)((char*)header + align_size(requested_size) + sizeof(size_t));
        // *new_footer = *footer;
        // printf("new footer: %p\n", new_footer);
        // printf("old footer: %p\n", footer);
        // printf("address of next_header: %p\n", next_header);

        // create a node pointer to the free node that was at the adress of the next header
        // create a node pointer to where the free node should move to
        struct Node* free_node = (struct Node*)next_header;
        struct Node* new_free_node = (struct Node*)((char*)new_footer + sizeof(size_t));

        // printf("address of free_node: %p\n", free_node);
        // printf("address of new_free_node: %p\n", new_free_node);
        // printf("free_node->prev: %p\n", free_node->prev);

        if (free_node->prev != NULL) {
            free_node->prev->next = new_free_node;  
            new_free_node->prev = free_node->prev;
        }

        new_free_node->next = free_node->next;
        new_free_node->next->prev = new_free_node; // this line might segfault haha i hope it doesnt

        // printf("reduced the free node size by %lu\n", requested_size-(*header & ~1)+16);
        new_free_node->size = free_node->size - (requested_size-(*header & ~1)+16);
        
        *header = requested_size + 2*sizeof(size_t) + 1;
        *new_footer = requested_size + 2*sizeof(size_t) + 1;

        // printf("new_free_node->size: %ld\n", new_free_node->size);

        return ptr;


    } else { // the next block is not free, or it doesnt have enough size. 
             // must copy all the data elsewhere
        
        // printf("Must copy the allocation somewhere else\n");

        // create a new node pointer to a new place in memory
        struct Node* new_ptr = mymalloc(requested_size);
        size_t original_payload_size = (*header & ~1) - 2*sizeof(size_t);

        printf("Copying %lu bytes of data from the original allocation at %p to the new allocation at %p\n", original_payload_size, ptr, new_ptr);
        printf("Freeing the original allocation at %p\n", ptr);
        
        
        memcpy(new_ptr, ptr, original_payload_size);
        myfree(ptr);
        return new_ptr;

    }


    // temp
    // return NULL;
}


void myfree(void* ptr) {
    if (ptr == NULL) return;

    //First, coalesce with blocks directly after and directly before if necessary

    //NOTE: need to add edge case where the next block is OUTSIDE list aka unreachable
    struct Node* curr_header = (void*)(char*)ptr-sizeof(size_t);

    //Gets the right adjacent block
    char* adjacent = (char *)curr_header + (curr_header->size & ~1);
    
    //Var for checking if coalescing occurred
    int coalescingOccurred = 0;

    //Two checks for right coalesce:
    //Edge case, if right block is not outside heap AKA reachable (don't coalesce if not)
    //Case in which lowest order bit is 0 to check if it's free
    if ((heap + MAX_HEAP) > adjacent && (((struct Node*)adjacent)->size & 1) == 0) {
        coalescingOccurred = 1;

        struct Node* adj_header = (void*)adjacent;
        //Currently assuming we have a head and tail node, remove right adj free block from free list
        struct Node* temp_next = adj_header->next;
        struct Node* temp_prev = adj_header->prev;

        (adj_header->prev)->next = temp_next;
        (adj_header->next)->prev = temp_prev;

        //Hypothetically, we would just overwrite the header when necessary, so we don't need to
        //clear the prev and next ptrs, but we do it here just in case
        adj_header->next = NULL;
        adj_header->prev = NULL;

        //Modify header & footer to contain correct size
        //Have to ensure that curr_header's size is correctly bitmasked, hence '& ~1'
        size_t new_size = ((curr_header->size) & ~1) + (adj_header->size);

        //Footer first, get ptr to footer
        void* footer = adjacent + (adj_header->size) - sizeof(size_t);
        *(size_t*)footer = new_size;

        //Header, we get direct access in curr_header since it's leftmost
        curr_header->size = new_size;
    }

    //Now coalesce with the left block if necessary
    char* left_footer = (char *)curr_header - sizeof(size_t);
    //Two checks for left coalescing:
    //If footer is outside heap then don't coalesce
    //Otherwise, if the footer's lowest-order bit is equal to 0, then it's free, so coalesce
    if (left_footer >= (heap) && (*(size_t*)left_footer & 1) == 0) {
        coalescingOccurred = 1;

        //adj_header is the curr_header pointer minus the footer val of the left block
        struct Node* adj_header = (void *)curr_header - (*(size_t*)left_footer & ~1);
        
        //remove left adj free block from free list
        struct Node* temp_next = adj_header->next;
        struct Node* temp_prev = adj_header->prev;
        adj_header->prev->next = temp_next;
        adj_header->next->prev = temp_prev;
        
        //Unnecessarily clear ptrs
        adj_header->next = NULL;
        adj_header->prev = NULL;

        //Modify header & footer to contain correct size
        //Have to ensure that curr_header's size is correctly bitmasked, hence '& ~1'
        size_t new_size = ((curr_header->size) & ~1) + (adj_header->size);

        //Header, we get direct access in adj_header since it's leftmost
        adj_header->size = new_size;

        //Footer, we can access it through curr_header plus its size
        //We need to do '& -1' here, just in case right coalescing didn't occur, and it still has a low-order bit of 1
        void* end_footer = (void*)curr_header + (curr_header->size & ~1) - sizeof(size_t);
        *(size_t*)end_footer = new_size;

        //Also make adj_header the new curr_header
        curr_header = adj_header;

    }

    //If coalescing didn't occur, then we manually have to edit the header and footer's size to have the corresponding lowest-order bit
    if (!coalescingOccurred) {
        curr_header->size = curr_header->size & ~1;
        size_t * footer = (void*)curr_header + (curr_header->size) - sizeof(size_t);
        *footer = curr_header->size;
    }
    

    //Finally, LIFO: move free block to front of the free list and fix free list ordering
    void* temp = head->next;
    head->next = curr_header;
    curr_header->prev = head;
    curr_header->next = temp;
    
    curr_header->next->prev = curr_header;
}


/*
 * align takes int x and 
 * aligns it to be a factor of 8
 *  - this is for aligning memory
 */
unsigned int align(unsigned int x){
    return 8 * ceil((double)x/8);
}

size_t align_size(size_t x){
    return (size_t)align((unsigned int)x);
}

/*
int main(int argc, char* argv[]){
    if(argc > 1) myinit(atoi(argv[1]));
    else myinit(0);
    printf("%d\n",heapAddress);

    printf("sizeof size_t: %ld\n", sizeof(size_t));

    void* t = mymalloc(9);
    printf("address of t: %p\n", t);

    void* w = mymalloc(10);
    printf("address of w: %p\n", w);

    // char* header = (char*)t-sizeof(size_t);
    // printf("value at header: %d\n", *header);

    void* d = myrealloc(t, 20);
    printf("address of d: %p\n", d);

    return 0;
}
*/
/*

int main_two(int argc, char* argv[]){
    myinit(atoi(argv[1]));
    printf("%d\n",heapAddress);
    // void* t;
    //  for(int i = 1; i < 64; i++){
    // //   mymalloc(i);
    //     t = malloc(i);
    //     if(i % 2 == 0) myfree(t);
    // }
    

    printf("Allocated all pieces\n");
    //if(true) exit(0);
    char* ptr = heap;
    
    //int count = 0;
    struct Node* temp = (struct Node*)ptr;
    while(temp->next != NULL){
        temp = (struct Node*)ptr;
        printf("POINTER: %d -> SIZE: %d \n", ptrAddr(ptr)-ptrAddr(head), (int)temp->size);
        printf("DIFFERENCE: %d \n", (ptrAddr(temp->next)-ptrAddr(ptr))); 
        ptr = ptr + temp->size;
        //if(count++ > 10) exit(0);
    }

    mycleanup();

    return 0;
}
*/

/*
 *firstFitSearch and bestFitSearch both assume
 *an aligned size(done in malloc) and that head points to the 
 *head of a properly created free list
 */


//this search will return the node that
//is the first fit in the list
struct Node* firstFitSearch(size_t size){
    //set ptr to traverse list
    struct Node* ptr = head->next;
    //while we're not at the end and the size of size is greater than ptr->size
    //we traverse the list
    while(ptr != tail && ptr->size < size){
        ptr = ptr->next;
    }

    //ptr will be the first available free space in the list
    //if there is no more free space, then null will be returned
    return ptr;
}

struct Node* bestFitSearch(size_t size){
    //set ptr to traverse list
    struct Node* ptr = head->next;
    //set bestSize to MAX_HEAP+1, so we know we'll never get a size bigger than heap
    size_t bestSize = MAX_HEAP*2;
    //initialize bestNode to be set later
    struct Node* bestNode = head->next;

    //traverse list to the end since we're searching for the best fit.
    while(ptr != tail){
        //if we find a ptr of size size, we break because that's perfect fit
        if(ptr->size == size){
            bestNode = ptr;
            break;
        }

        //if size can be contained in ptr->size
        if(ptr->size > size){
            //if size is also smaller than best size
            if(size < bestSize){
                //set bestsize to the new size, and update bestnode to be
                //ptr, the best current node
                bestSize = ptr->size;
                bestNode = ptr;
            }
        }
        ptr = ptr->next;
    }
    //bestNode will be the best node we could find OR
    //it will be null if there is no space
    return bestNode;
}

void printHeap() {
    void* curr = heap;
    printf("\n\tITERATING THROUGH HEAP:\n\n");
    
    printf("\thead->next = %p\thead->prev = %p\n", head->next, head->prev);
    printf("\ttail->next = %p\ttail->prev = %p\n", tail->next, tail->prev);
    printf("\n");


    while ((char*)curr < (heap + MAX_HEAP)) {

        struct Node* currHeader = curr;
        
        printf("\tStart of block: %p\n", curr);
        printf("\tHeader: %lu\n", (currHeader->size));
        printf("\tActual Size: %lu\n", (currHeader->size & ~1));
        printf("\tFooter: %lu\n", *(size_t*)(curr + (currHeader->size & ~1) - sizeof(size_t)));

        if ((currHeader->size & 1) == 1) {
            printf("\tAllocated\n");
        } else {
            printf("\tFree\n");
            printf("\tNext: %p\n",currHeader->next);
            printf("\tPrev: %p\n",currHeader->prev);
        }
        printf("\n");

        curr += (currHeader->size & ~1);
    }

    printf("\tFinal location: %p\n\n", curr);
}

/*
int main(int argc, char* argv[]){
    myinit(0);
    void* a = mymalloc(18);
    void* b = mymalloc(71);
    void* c = mymalloc(2003);
    printHeap();
    printf("Pointer a: %p\n", a);
    printf("Pointer b: %p\n", b);
    printf("Pointer c: %p\n", c);
    myfree(b);
    printHeap();
    myfree(a);
    printHeap();
    myfree(c);
    printHeap();
    mycleanup();
}
*/


// malloc'ing a nonzero size produces a block with correct header and footer.
// should be alloc_algo agnostic.
void test1() {
    void* t = mymalloc(16);
    size_t* t_header = (size_t*)(t-sizeof(size_t));
    size_t* t_footer = (size_t*)((char*)t_header + 16 + sizeof(size_t));
    assert(*t_header == 33);
    assert(*t_footer == 33);

    void* d = mymalloc(100);
    size_t* d_header = (size_t*)(d-sizeof(size_t));
    size_t* d_footer = (size_t*)((char*)d_header + 104 + sizeof(size_t));
    assert(*d_header == 121);
    assert(*d_footer == 121);

    void* f = mymalloc(1);
    size_t* f_header = (size_t*)(f-sizeof(size_t));
    size_t* f_footer = (size_t*)((char*)f_header + 16 + sizeof(size_t));
    assert(*f_header == 33);
    assert(*f_footer == 33);

    printf("Passed Test 1.\n");
}

// allocating blocks and freeing all of them should result in a single free block
// with a header that has a size value of the max heap.
// this tests coalescing and myfree() as a whole.
// assumes first fit search algo.
void test2() {
    void* t = mymalloc(8);
    size_t* t_header = (size_t*)(t-sizeof(size_t));
    size_t* t_footer = (size_t*)((char*)t_header + 16 + sizeof(size_t));

    printf("t_header: %p\n", t_header);
    printf("value at t_header: %lu\n", *t_header);
    printf("t_footer: %p\n", t_footer);
    printf("value at t_footer: %lu\n", *t_footer);

    assert(*t_header == 33);
    assert(*t_footer == 33);

    void* d = mymalloc(8);
    void* h = mymalloc(32);

    myfree(t);

    assert(*t_header == 32);
    assert(*t_footer == 32);

    myfree(d);
    myfree(h);
    
    // after everything is freed, t_header should point to the header of the free block
    assert(*t_header == MAX_HEAP);

    printf("Passed Test 2.\n");
}

// testing realloc
void test3() {
    void* h = mymalloc(32);
    size_t* h_header = (size_t*)(h-sizeof(size_t));

    assert(*h_header == 49);

    h = myrealloc(h, 0);

    assert(*h_header == MAX_HEAP);

    h = myrealloc(h, 32);

    assert(*h_header == 49);

    h = myrealloc(h, 64);
    size_t* h_footer = (size_t*)((char*)h_header + 64 + sizeof(size_t));
    // printf("h_footer: %p\n", h_footer);
    // printf("*h_footer: %ld\n", *h_footer);
    assert(*h_header == 81);
    assert(*h_footer == 81);

    void* p = mymalloc(32);
    size_t* p_header = (size_t*)(p-sizeof(size_t));
    assert(*p_header == 32+16+1);
    
    h = myrealloc(h, 128); // original allocation is freed, data copied to new location
    size_t* new_h_header = (size_t*)(h-sizeof(size_t));
    assert(*h_header == 80); 
    assert(*new_h_header == 145);
    assert(new_h_header == (size_t*)((char*)p+32+8)); // new header is right after p allocation

    assert(*h_header == 80);
    myfree(p);
    assert(*h_header == 128); // when p is freed, it coalesces with free block at the start
    myfree(h);

    // printf("*h_header: %lu\n", *h_header);
    assert(*h_header == MAX_HEAP);

    printf("Passed Test 3.\n");

}


// similar to test 3
void test4() {;
    // printf("head: %lu\n", head->next->size);
    assert(head->next->size == MAX_HEAP);

    size_t* p = mymalloc(32);
    assert(head->next->size == MAX_HEAP - (32+16));

    p = myrealloc(p, 64);
    assert(head->next->size == MAX_HEAP - (64+16));

    myfree(p);
    assert(head->next->size == MAX_HEAP);

    printf("Passed Test 4.\n");
}

// allocate 3 in a row, free the middle one, then allocate again. new malloc should end up in the middle again.
// assumes first fit search
void test5() {
    void* p = mymalloc(32);
    void* y = mymalloc(32);
    void* x = mymalloc(32);

    myfree(y);

    void* w = mymalloc(32);

    assert(w == y);

    myfree(p);
    myfree(w);
    myfree(x);

    printf("Passed Test 5.\n");
}


int main() {
    // each test assumes a clean slate
    myinit(0);
    test1();
    mycleanup();

    myinit(0);
    test2();
    mycleanup();

    myinit(0);
    test3();
    mycleanup();

    myinit(0);
    test4();
    mycleanup();

    myinit(0);
    test5();
    mycleanup();

    return 0;
}
