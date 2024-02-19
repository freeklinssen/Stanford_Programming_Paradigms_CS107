#include "hashset.h"
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>

void HashSetNew(hashset *h, int elemSize, int numBuckets,
		HashSetHashFunction hashfn, HashSetCompareFunction comparefn, HashSetFreeFunction freefn)
{
	assert(elemSize>0);

	h->numBuckets = numBuckets;
	h->hashfn = hashfn;
	h->comparefn = comparefn;
	h->freefn = freefn;

	h->buckets = malloc(numBuckets * sizeof(vector));
	for(int i=0; i<numBuckets; i++)
	{
		VectorNew(&h->buckets[i], elemSize, freefn, 8);
	}	
}


void HashSetDispose(hashset *h)
{
	if(h->freefn != NULL)
	{
		for(int i=0; i<h->numBuckets; i++)
		{
			VectorDispose(&h->buckets[i]);
		} 
	}
	free(h->buckets);
}


int HashSetCount(const hashset *h)
{
	int count = 0;
	for(int i=0; i<h->numBuckets; i++)
	{
		count += VectorLength(&h->buckets[i]);
	} 
	return count; 
}


void HashSetMap(hashset *h, HashSetMapFunction mapfn, void *auxData)
{
	for(int i=0; i<h->numBuckets; i++)
	{
		VectorMap(&h->buckets[i], mapfn, auxData);
	}
}


void HashSetEnter(hashset *h, const void *elemAddr)
{
	assert(elemAddr !=0);
	int bucket = h->hashfn(elemAddr, h->numBuckets);
	assert(0<= bucket <h->numBuckets);
	
	int index = VectorSearch(&h->buckets[bucket], elemAddr, h->comparefn, 0, false);
	if(index == -1)
	{
		VectorAppend(&h->buckets[bucket], elemAddr);
	}
	else
	{
		VectorReplace(&h->buckets[bucket], elemAddr, index);
	}
}


void* HashSetLookup(const hashset *h, const void *elemAddr)
{ 
	int bucket = h->hashfn(elemAddr, h->numBuckets);
    
	int index = VectorSearch(&h->buckets[bucket], elemAddr, h->comparefn, 0, false);
	if(index != -1)
	{
		return VectorNth(&h->buckets[bucket], index);
	}
	else{return NULL;}
}
