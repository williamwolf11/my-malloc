/*
 * my-malloc.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <limits.h>
#include <stdint.h>
#include <string.h>

#define INTERNAL_SIZE_T size_t
#define TRUE 1
#define FALSE 0
#define ALIGNMENT_SIZE 16
#define PAGE_SIZE 1000

/*initialize total break space to 0*/
int total_break_space = 0;

typedef struct allocation {
    int offset_to_end; /*Size of the full allocation (including metadata)*/
    struct allocation * next; /*Pointer to the next allocation*/
    struct allocation * prev; /*Pointer to the previous allocation*/
    char padding[8];
} allocation;

/*Prototypes*/
void * malloc(size_t size);
void free(void *ptr);
void * calloc(size_t count, size_t size);
void * realloc(void *ptr, size_t size);
int malloc_usable_size(void *ptr);
void head_allocation(int size);
void before_head_allocation(int size, allocation* head);
void middle_allocation(allocation *prev_allocation, int size, allocation *next_allocation);
void end_allocation(allocation *prev_allocation, int size);
int inc_break(int size);

/*initial instances of struct allocation*/
static allocation *head;
static allocation *break_start;

void * malloc(size_t size){
    /* Returns a void pointer to the start of the allocated memory chunk just as malloc does */
    allocation *curr_allocation;

    int remainder_size = size % ALIGNMENT_SIZE;
    int rounded_size = ((2 * sizeof(allocation)) + size + (ALIGNMENT_SIZE - remainder_size));
    /* Returns a long integer of the pointer to the start of the allocated memory*/

    /* if head has not been initialized to be the first allocation pointer */
    if (head == NULL && total_break_space == 0) {

        head = sbrk(0); /*initialize the head to be at the bottom of the stack*/
        break_start = head;

        /*If the requested size is larger then the current total space*/
        total_break_space += inc_break(rounded_size);

        /* Return a pointer to the start of the stack, i.e. where the head has
        been initialized to */
        head_allocation(rounded_size);
        return (void *) (((char *) head) + sizeof(allocation));
    }
    else if (head == NULL && total_break_space != 0) {
        /*If the requested size is larger then the current total space*/

        if (rounded_size > total_break_space){
            total_break_space += inc_break(rounded_size);
        }

        /* Return a pointer to the start of the stack, i.e. where the head has
        been initialized to */
        head = break_start;
        head_allocation(rounded_size);
        return (void *) (((char *) head) + sizeof(allocation));
    }
    else{
        /* When the head has been initialized we need to iterate through the entire LL
        * and check if there is enough space between, pointers or at the end of
        * the last pointer */

        curr_allocation = head;

        /* if there is space before the head to add it, add it there */
        if (rounded_size < ((char *) head) - ((char *) break_start)){
            before_head_allocation(rounded_size, head);
            return (void *) (((char *) break_start) + sizeof(allocation));
        }


        /* Traversing the LL*/
        while (curr_allocation->next != NULL){
            /* check if there is enough space between the pointer to the
            current allocation iterator + its size to the next allocation */

            /*ADDING A ALLOCATION WITHIN A GAP IN THE LL, MEANING THERE ARE ITEMS AFTER THE NEW ALLOCATION*/
            if ((((char *) curr_allocation) + curr_allocation->offset_to_end - ((char *) curr_allocation->next) > rounded_size)){
                middle_allocation(curr_allocation, rounded_size, curr_allocation->next);
                return (void *) (((char *) curr_allocation) + curr_allocation->offset_to_end + sizeof(allocation));
            }
            else {
                curr_allocation = curr_allocation->next;
            }

        }

        /* see if there is enough space left in the buffer after the
        last allocation to store new allocation */

        /* IF WE HAVE ITERATED TO END AND THERE IS SPACE IN THE BREAK TO APPEND AN ALLOCATION*/
        if (((char *) curr_allocation) - ((char *) break_start) + curr_allocation->offset_to_end + rounded_size < total_break_space){
            end_allocation(curr_allocation, rounded_size);
            return (void *) (((char *) curr_allocation) + curr_allocation->offset_to_end + sizeof(allocation));
        }
        else {
            /* IF WE HAVE ITERATED TO END AND THERE IS NOT SPACE IN THE BREAK TO APPEND A ALLOCATION*/

            /*If the requested size is larger then the current total space*/
            total_break_space += inc_break(rounded_size);

            end_allocation(curr_allocation, rounded_size);
            return (void *) (((char *) curr_allocation) + curr_allocation->offset_to_end + sizeof(allocation)); /*Meaning there is not enough space in
            the current list with buffer size to put the requested space*/
        }
        return NULL;
    }
}

void free(void *ptr){
    allocation *freed_allocation;

    /* Free, when given a null pointer, does nothing */
    if (ptr != NULL){

        /* Get to the beginning of the metadata before the allocation by casting to alloc struct and subtracting one */
        freed_allocation = (allocation *) (((char *) ptr ) - sizeof(allocation));

        if (freed_allocation->prev != NULL && freed_allocation->next != NULL){
            /* yes prev and yes next */
            freed_allocation->prev->next = freed_allocation->next;
            freed_allocation->next->prev = freed_allocation->prev;
        }
        else if (freed_allocation->prev == NULL && freed_allocation->next != NULL){
            /* no prev and yes next */
            freed_allocation->next->prev = NULL;
            head = freed_allocation->next;
        }
        else if(freed_allocation->prev != NULL && freed_allocation->next == NULL){
            /* yes prev and no next */
            freed_allocation->prev->next = NULL;
        }
        else {
            /* no prev and no next */
            head = NULL;
        }
    }
}

void * calloc(size_t count, size_t size){
    /* Allocate unused space for an array of count elements whose size are each size,
    then initialize all bits to 0 */
    void * new_ptr;
    if (count == 0 || size == 0){
        return NULL;
    }
    if (count > INT_MAX / size) {
        return NULL;
    }

    new_ptr = malloc(count*size);
    if (new_ptr == NULL){
        return NULL;
    }
    memset(new_ptr, 0, count*size);
    return new_ptr;
}

void * realloc(void *ptr, size_t new_size){
    /*Deallocate the old object pointed to by ptr and return a pointer to a new object
    of size new_size*/
    allocation *re_allocation;
    /* Get to the beginning of the metadata before the reallocation */
    re_allocation = (allocation *) (((char *) ptr ) - sizeof(allocation));
    void *new_ptr;

    if (new_size == 0){
        free(ptr);
        return NULL;
    }
    else if (ptr == NULL){
        return malloc(new_size);
    }
    else if (re_allocation->offset_to_end >= new_size){
        return ptr;
    }
    else{
        new_ptr = malloc(new_size);
        if (new_ptr == NULL){
            return new_ptr;
        }
        memcpy(new_ptr, ptr, re_allocation->offset_to_end);
        free(ptr);
        return new_ptr;
    }
}

int malloc_usable_size(void *ptr){
    /*returns the number of usable bytes in the block pointed to by ptr*/

    if (ptr == NULL){
        return 0;
    }
    allocation *alloc = (allocation *) (((char *) ptr ) - sizeof(allocation));
    return alloc->offset_to_end - sizeof(allocation);
}

void head_allocation(int size){
    /*Make an allocation at the head of the list, or bottom of the stack*/
    allocation *curr_allocation;

    curr_allocation = break_start;

    /*Assignments*/
    curr_allocation->prev = NULL;
    curr_allocation->offset_to_end = size;
    curr_allocation->next = NULL;
}

void before_head_allocation(int size, allocation *head){
  /*special case where we make an allocation before the head in our
  linked list, and subsequently make it the head*/
    allocation *curr_allocation;
    curr_allocation = break_start;

    /*Assignments*/
    curr_allocation->prev = NULL;
    curr_allocation->offset_to_end = size;
    curr_allocation->next = head;

    head-> prev = curr_allocation;
    head = curr_allocation;
}

void middle_allocation(allocation *prev_allocation, int size, allocation *next_allocation){
    /*Make an allocation between two allocations already in the linked list*/
    allocation *curr_allocation;

    curr_allocation = (allocation *)(((char *) prev_allocation) + prev_allocation->offset_to_end);

    /*Assignments to prev*/
    prev_allocation->next = curr_allocation;

    /*Assignments to curr*/
    curr_allocation->prev = prev_allocation;
    curr_allocation->offset_to_end = size;
    curr_allocation->next = next_allocation;

    /*Assignments to next*/
    next_allocation->prev = curr_allocation;
}

void end_allocation(allocation *prev_allocation, int size){
    /*Make an allocation at the end of the listm*/
    allocation *curr_allocation;
    curr_allocation = (allocation *)(((char *) prev_allocation) + prev_allocation->offset_to_end);

    /*Assignments to prev*/
    prev_allocation->next = curr_allocation;

    /*Assignments to curr*/
    curr_allocation->prev = prev_allocation;
    curr_allocation->offset_to_end = size;
    curr_allocation->next = NULL;
}

int inc_break(int size){
    /* rounds the incremented break size to a multiple of the page size */
    if (size > PAGE_SIZE){
        int remainder_size = size % PAGE_SIZE;
        sbrk(size + (PAGE_SIZE - remainder_size));
        return size + (PAGE_SIZE - remainder_size);
    }
    else {
        sbrk(PAGE_SIZE);
        return PAGE_SIZE;
    }
}
