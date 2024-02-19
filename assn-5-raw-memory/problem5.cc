


void* start_heap;
int heap_size;

hakHeapStart(start_heap); 
kHeapSize (heap_size); 

#include <stdio.h>

#define ARRAY_LENGTH(arr) (sizeof(arr) / sizeof((arr)[0]))

struct list {
    int data;
    struct list *next;
};

bool findListInHeap()
{
    int answer[3] = {1,2,3};
    list Try; 

    for(int i =0; i<(heap_size-sizeof(list)); i++)
    {
        Try = *(struct list*) ((char*)start_heap + i);

        for(int j=0; j<ARRAY_LENGTH(answer); j++)
        {
            if(Try.data == answer[j])
            {
                if(j == (ARRAY_LENGTH(answer)-1 && Try.next==NULL))
                {
                    return true;
                }
                if(Try.next !=NULL && start_heap>=Try.next && Try.next<= (void*)((char*)start_heap + heap_size-sizeof(list)))
                {
                    Try =  *(struct list*) Try.next;
                }
                else{break;}
            } 
            else{break;}
        }
    }

    return false;
}

//make the original list we are looking for


bool present = findListInHeap();

