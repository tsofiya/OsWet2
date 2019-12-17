
#include <unistd.h>
#include <cstring>
#include <iostream>
#include "smalloc.h"


struct MallocMetadata{
    size_t size;
    bool is_free;
    MallocMetadata* next;
    MallocMetadata* prev;
};

//Global variables:
MallocMetadata* BlockList=NULL;
MallocMetadata* BlockListTail=NULL;
size_t free_blocks=0;
size_t free_bytes=0;
size_t allocated_blocks=0;
size_t allocated_bytes=0;

//Functions:
static void* getStart(MallocMetadata* block){
    return (block+ sizeof(MallocMetadata));
}

static size_t getSize(MallocMetadata* block);
static MallocMetadata* findBlock(size_t s){
    MallocMetadata *ptr= BlockList;
    while (ptr!=NULL){
        if (ptr->is_free==true && s<=ptr->size){
            return ptr;
        }
        ptr=ptr->next;
    }
    return NULL;
}


static bool UnFree(MallocMetadata* block){
    block->is_free=false;
    free_blocks--;
    free_bytes=free_bytes-block->size;
}



static MallocMetadata* getMetaDataByPointer(void* mem){ //this func causes SEGFAULT
    MallocMetadata* ptr= BlockList;
    while(ptr!=NULL){
        if (ptr+sizeof(MallocMetadata)==mem)//TODO: ask about ++
            return ptr;
        ptr= ptr->next;
    }
    return NULL;
}


static MallocMetadata * allocateMetadataAndMem(size_t s){
    if (s<0 || s>100000000){
        return NULL;
    }
    void * ptr= sbrk(s+sizeof(MallocMetadata));
    if((ptr==(void*)(-1))){
        return NULL;
    }

    allocated_bytes+= s;
    allocated_blocks++;

    //ptr=sbrk(0); //addition
    MallocMetadata * metadata = (MallocMetadata*)ptr;
    metadata->size= s;
    metadata->is_free= false;
    if (BlockList==NULL) {
        BlockList= metadata;
        BlockListTail= metadata;
        metadata->next=NULL;
        metadata->prev=NULL;
    }
    else{
        BlockListTail->next= metadata;
        metadata->prev= BlockListTail;
        metadata->next=NULL;
        BlockListTail= metadata;
    }
    return metadata;
}

void freeMetaData(MallocMetadata* metadata){
    metadata->is_free=true;
    free_blocks++;
    free_bytes=free_bytes+(metadata->size);
}


void* smalloc(size_t size){

    unsigned int s=size; //check conversion
    if (s<=0 || size>100000000){
        return NULL;
    }
    void* ptr;
    MallocMetadata* block= findBlock(size);
    if(!block){
        block=allocateMetadataAndMem(size);
    }
    else{
        UnFree(block);
    }
    ptr=getStart(block);
    return ptr;


}


void sfree (void* p){
    if(!p){
        return;
    }
    MallocMetadata* metadata=getMetaDataByPointer(p);
    if(metadata->is_free){
        return;
    }
    freeMetaData(metadata);
//TODO: memset?
}


size_t _num_free_blocks(){
    return free_blocks;
}


size_t _num_allocated_blocks(){
    return allocated_blocks;
}


size_t _num_meta_data_bytes(){
    return (allocated_blocks)*sizeof(MallocMetadata);
}

void* scalloc(size_t num, size_t size){
    bool prev_free=true;
    MallocMetadata* metadata= findBlock(num*size);
    if (metadata== NULL){
        prev_free=false;
        metadata= allocateMetadataAndMem(num*size);
        if (metadata==NULL)
            return NULL;
    }
    if(prev_free){
        free_blocks--;
        free_bytes=free_bytes-metadata->size;
    }
    return memset(getStart(metadata), 0, num*size);
}

void* srealloc(void* oldp, size_t size){
    if (size==0)
        return NULL;

    if (oldp==NULL) {
        return smalloc(size);
    }

    MallocMetadata* oldMetaData= getMetaDataByPointer(oldp);
    if (oldMetaData->size>=size) {
        return (oldMetaData + sizeof(MallocMetadata));
    }
    MallocMetadata* newMetaData= findBlock(size);
    if (newMetaData==NULL) {
        newMetaData = allocateMetadataAndMem(size);
    }
    memcpy((void*)(getStart(newMetaData)), (void*) (getStart(oldMetaData)), oldMetaData->size);
    oldMetaData->is_free= true;
    freeMetaData(oldMetaData);
    return (newMetaData+sizeof(MallocMetadata));
}


size_t _num_free_bytes(){
    return free_bytes;
}

size_t _num_allocated_bytes(){
    return allocated_bytes;
}

size_t _size_meta_data(){
    return sizeof(MallocMetadata);
}

void printBlockList(){
    MallocMetadata* current= BlockList;
    while(current!=NULL){
        std::cout<< "Metadata size: " << current->size <<" metadata is free? "<< current->is_free<<std::endl;
        current=current->next;
    }
    std::cout<< "Done!" <<std::endl;
}

