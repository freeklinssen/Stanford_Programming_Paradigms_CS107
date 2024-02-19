#include "vector.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

void VectorNew(vector *v, int elemSize, VectorFreeFunction freeFn, int initialAllocation)
{
    v->elems = malloc(elemSize * initialAllocation);
    v->elemSize = elemSize;
    v->loglength = 0;
    v->alloclength = initialAllocation;
    v->freeFn = freeFn;
    assert(v->elems != NULL);
}



void VectorDispose(vector *v)
{
    if(v->freeFn != NULL)
    {
        for(int i=0; i<v->loglength; i++)
        {
            v->freeFn(((char*)v->elems+i*v->elemSize));
        }
    }
    free(v->elems);
    v->loglength = 0;
}



int VectorLength(const vector *v)
{ return v->loglength; }



void *VectorNth(const vector *v, int position)
{ 
    assert(0 < position < v->loglength);
    return (void*)((char*)v->elems + position * v->elemSize); 
}



void VectorReplace(vector *v, const void *elemAddr, int position)
{
    assert(0 < position < v->loglength);
    if(v->freeFn != NULL)
    {
        v->freeFn((char*)v->elems+(position*v->elemSize));
    }

    void* destination = (char*)v->elems+position*v->elemSize;
    memcpy(destination, elemAddr, v->elemSize);
}



void VectorInsert(vector *v, const void *elemAddr, int position)
{
    assert(0 < position <= v->loglength);
    if(v->loglength == v->alloclength)
    {
      v->alloclength= v->alloclength*2;
      v->elems = realloc(v->elems, v->elemSize * v->alloclength);
    }
    
    if(position == v->loglength)
    {
        void* destination = (char*)v->elems+position*v->elemSize;
        memcpy(destination, elemAddr, v->elemSize);
    } 
    else
    {
        void* source = (char*)v->elems + position * v->elemSize;
        void* destination = (char*)v->elems + (position+1) * v->elemSize; 
        int size = (v->loglength-position)*v->elemSize; 
        memmove(destination, source, size);
        
        memcpy((char*)v->elems+position*v->elemSize, elemAddr, v->elemSize);
    }
    v->loglength++;
}


void VectorAppend(vector *v, const void *elemAddr)
{  
    if(v->loglength == v->alloclength)
    {
      v->alloclength = v->alloclength*2;
      v->elems = realloc(v->elems, v->elemSize * v->alloclength);
      assert(v->elems != NULL);
    }

    void* destination = (char*)v->elems+v->loglength*v->elemSize;
    memcpy(destination, elemAddr, v->elemSize);
    v->loglength++;
}


void VectorDelete(vector *v, int position)
{
    assert(0 < position < v->loglength);
    if(v->freeFn != NULL)
    {
        v->freeFn((char*)v->elems+(position*v->elemSize));
    }
    if(position == (v->loglength-1))
    {
        v->loglength--;
    }   
    else
    {
        void* source = (void*)((char*)v->elems + (position+1) * v->elemSize);
        void* destination = (void*)((char*)v->elems + (position) * v->elemSize); 
        int size = (v->loglength-(position+1))*v->elemSize; 
        memmove(destination, source, size);
        //memcopy when the two regions do not voverlap, but they do in this case
        v->loglength--;
    }
}


void VectorSort(vector *v, VectorCompareFunction compare)
{ 
    assert(compare != NULL);
    qsort(v->elems, v->loglength, v->elemSize, compare);
}


void VectorMap(vector *v, VectorMapFunction mapFn, void *auxData)
{
    for(int i=0; i<v->loglength; i++)
    {
        mapFn((void*)((char*)v->elems+i*v->elemSize), auxData);
    }
}


static const int kNotFound = -1;
int VectorSearch(const vector *v, const void *key, VectorCompareFunction searchFn, int startIndex, bool isSorted)
{ 
    int index = -1;
    if(isSorted)
    {
        int low = 0;
        int high = v->loglength-1;
        int mid = (v->loglength-1)/2;

        while(true)
        {
            int result = searchFn((char*)v->elems+mid*v->elemSize, key);
            if(result==0)
            {
            return mid;
            }

            if(result>0)
            {
            if(mid==low) {break;}
            high = mid-1;
            mid = (low-1) + ((mid-low)+1)/2;
            }
            else
            {
            if(mid==high) {break;}
            low = mid+1;
            mid = mid + ((high-mid)+1)/2;
            }
        }
    }
    else
    {
        for(int i = startIndex; i<v->loglength; i++)
        {
            if(searchFn((char*)v->elems+i*v->elemSize, key) == 0)
            {
                return i;
            }
        }
    }
    return index; 
} 
