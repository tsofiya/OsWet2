
#include <unistd.h>
#include <cstring>
#include <sys/mman.h>
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

MallocMetadata* getListTail(){

    MallocMetadata* current=BlockList;
    if(current==NULL){
        return NULL;
    }
    while (current->next!=NULL){
        current=current->next;
    }
    return current;
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
        MallocMetadata* tail=getListTail();
        tail->next= metadata;
        metadata->prev= tail;
        metadata->next=NULL;
        BlockListTail= metadata;
    }
    return metadata;
}



void * allocateMmap(size_t s){
    void * ptr= mmap(0, s+ sizeof(unsigned long long), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
    if((ptr==(void*)(-1))){
        return NULL;
    }

    unsigned long long * l = (unsigned long long*)ptr;
    *l=s;
    allocated_blocks++;
    allocated_bytes+=s;
    return ptr;
}

void freeMetaData(MallocMetadata* metadata){
    metadata->is_free=true;
    free_blocks++;
    free_bytes+=metadata->size;
    if(metadata->prev==NULL && metadata->next==NULL){
        return;
    }
    if (metadata->prev != NULL && metadata->prev->is_free && metadata->next!= NULL && metadata->next->is_free){
        metadata->prev->size+=metadata->next->size + metadata->size + 2* sizeof(MallocMetadata);
        metadata->prev->next= metadata->next->next;
        if(metadata->next->next!=NULL) {
            metadata->next->next->prev = metadata->prev;
        }
        free_bytes+=2* sizeof(MallocMetadata);
        allocated_bytes+=2*sizeof(MallocMetadata);
        free_blocks=free_blocks-2;
        allocated_blocks=allocated_blocks-2;
    }
    else if(metadata->prev != NULL && metadata->prev->is_free){
        metadata->prev->size+= metadata->size+sizeof(MallocMetadata);
        metadata->prev->next= metadata->next;
        if(metadata->next!=NULL) {
            metadata->next->prev = metadata->prev;
        }
        free_bytes+=sizeof(MallocMetadata);
        allocated_bytes+=sizeof(MallocMetadata);
        free_blocks--;
        allocated_blocks--;
    }else if(metadata->next != NULL && metadata->next->is_free){
        metadata->size+= metadata->next->size+sizeof(MallocMetadata);
        metadata->next= metadata->next->next;
        if(metadata->next!=NULL) {
            metadata->next->prev = metadata;
        }
        free_bytes+=sizeof(MallocMetadata);
        allocated_bytes+=sizeof(MallocMetadata);
        free_blocks--;
        allocated_blocks--;

    }
}

MallocMetadata* split(MallocMetadata* block, size_t size){
    size_t total_size= block->size;
    unsigned long long new_block_v= ( (unsigned long long) block + (unsigned long long) sizeof(MallocMetadata) + (unsigned long long)size);
    void* current_sbrk= sbrk(0);
    MallocMetadata* new_block=(MallocMetadata*) new_block_v;
    new_block->size=total_size-size-sizeof(MallocMetadata);
    new_block->is_free=true;
    new_block->next=block->next;
    new_block->prev=block;
    block->next=new_block;
    block->size=size;
    allocated_blocks++;
    allocated_bytes=allocated_bytes-sizeof(MallocMetadata);
    free_blocks++;
    free_bytes=free_bytes-sizeof(MallocMetadata);
    return block;
}

MallocMetadata* enlargeWilderness(size_t s){
    if (s<0 || s>100000000){
        return NULL;
    }
    MallocMetadata* tail=getListTail();
    size_t addition=s-tail->size;
    void * ptr= sbrk(addition);
    if((ptr==(void*)(-1))){
        return NULL;
    }

    allocated_bytes+= addition;
    tail->size=s;
    tail->is_free= false;
    BlockListTail=tail;
    return BlockListTail;
}

void* smalloc(size_t size) {
    unsigned int s = size; //check conversion
    if (s <= 0 || size > 100000000) {
        return NULL;
    }
    if (s >= 131072) {
        unsigned long long *ptr = (unsigned long long *) allocateMmap(s);
        return ptr++;
    } else {
        void *ptr;
        MallocMetadata *block = findBlock(size);
        if (!block) {
            MallocMetadata* tail=getListTail();
            if (tail != NULL && tail->is_free == true) {
                int tail_size=tail->size;
                block = enlargeWilderness(size);
                //  UnFree(block);
                free_blocks--;
                free_bytes=free_bytes-tail_size;
            } else {
                block = allocateMetadataAndMem(size);
                //UnFree(block);
            }
        } else {
            if (block->size >= 128 + sizeof(MallocMetadata) + size) {
                MallocMetadata *block_n = split(block, size);
                block = block_n;
                //free_blocks++;
            }
            UnFree(block);
        }

        ptr = getStart(block);
        return ptr;
    }
}



void sfree (void* p){
    if(!p){
        return;
    }
    MallocMetadata* metadata=getMetaDataByPointer(p);
    if (metadata==NULL){
        unsigned long long* ptr= (unsigned long long*)p;
        // ptr--;
        allocated_bytes=allocated_bytes-(*ptr);
        allocated_blocks--;
        munmap(ptr, *ptr);
        return;
    }
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
        metadata->is_free=false;
    }
    return memset(getStart(metadata), 0, num*size);
}


void* srealloc(void* oldp, size_t size) {
    if (oldp == NULL){
        return smalloc(size);
    }
    if (size==0) {
        return NULL;
    }
    MallocMetadata* oldMetaData= getMetaDataByPointer(oldp);
    if (oldMetaData->size>=size) {
        return (oldMetaData + sizeof(MallocMetadata));
    }
    if(oldMetaData->next!=NULL && oldMetaData->next->is_free && (oldMetaData->size+oldMetaData->next->size>=size)){
        size_t free_size=oldMetaData->next->size;
        (oldMetaData->size)+=oldMetaData->next->size +sizeof(MallocMetadata);
        oldMetaData->next=oldMetaData->next->next;
        if(oldMetaData->next->next!=NULL){
            oldMetaData->next->next->prev=oldMetaData;
        }
        allocated_blocks--;
        free_blocks--;
        allocated_bytes+=sizeof(MallocMetadata);
        free_bytes=free_bytes-free_size;
        if (oldMetaData->size >=150+size){
            size_t freebytes=oldMetaData->size-size-sizeof(MallocMetadata);
            split(oldMetaData, size);
            free_bytes+=oldMetaData->next->size;
            oldMetaData->next->is_free=true;
        }
        return (oldMetaData+sizeof(MallocMetadata));
    }
    else if(oldMetaData->prev!=NULL && oldMetaData->prev->is_free && (oldMetaData->size+oldMetaData->prev->size>=size)){
        size_t free_size=oldMetaData->prev->size;
        (oldMetaData->prev->size)+=oldMetaData->size +sizeof(MallocMetadata);
        oldMetaData->prev->next=oldMetaData->next;
        if(oldMetaData->next!=NULL){
            oldMetaData->next->prev=oldMetaData->prev;
        }
        allocated_blocks--;
        free_blocks--;
        allocated_bytes+=sizeof(MallocMetadata);
        free_bytes=free_bytes-free_size;
        oldMetaData->prev->is_free=false;
        memcpy(getStart(oldMetaData->prev), getStart(oldMetaData), oldMetaData->size);
        if (oldMetaData->prev->size >=150+size){

            split(oldMetaData->prev, size);
            free_bytes+=oldMetaData->prev->next->size;
            oldMetaData->prev->next->is_free=true;
        }
        return (getStart(oldMetaData->prev));
    }


    else if (oldMetaData->prev!=NULL && oldMetaData->prev->is_free && oldMetaData->next!=NULL && oldMetaData->next->is_free &&(oldMetaData->size+oldMetaData->prev->size+oldMetaData->next->size>=size)){
        size_t free_size=oldMetaData->next->size+oldMetaData->prev->size;
        oldMetaData->prev->size+=oldMetaData->size+oldMetaData->next->size+2*sizeof(MallocMetadata);
        oldMetaData->prev->next=oldMetaData->next->next;
        if (oldMetaData->next->next!=NULL){
            oldMetaData->next->next->prev=oldMetaData->prev;
        }
        free_blocks=free_blocks-2;
        allocated_bytes=allocated_bytes+2*sizeof(MallocMetadata);
        free_bytes= free_bytes-free_size;
        allocated_blocks=allocated_blocks-2;
        oldMetaData->prev->is_free=false;
        memcpy(getStart(oldMetaData->prev), getStart(oldMetaData), oldMetaData->size);
        if (oldMetaData->prev->size >=150+size){

            split(oldMetaData->prev, size);
            free_bytes+=oldMetaData->prev->next->size;
            oldMetaData->prev->next->is_free=true;
        } //not sure if they meant size here
        return (getStart(oldMetaData->prev));
    }


    MallocMetadata* tail=getListTail();
    if(tail!=NULL && tail==oldMetaData){ //Wilderness case
        MallocMetadata* newMetaData = enlargeWilderness(size);
        return (newMetaData+sizeof(MallocMetadata));

    }

    MallocMetadata* newMetaData= findBlock(size);
    if (newMetaData==NULL) {
        newMetaData = allocateMetadataAndMem(size);
    }
    if(newMetaData==NULL){
        return NULL;
    }
    memcpy(getStart(newMetaData), getStart(oldMetaData), oldMetaData->size-32);
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
