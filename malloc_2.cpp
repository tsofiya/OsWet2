//
// Created by student on 12/10/19.
//

#include <unistd.h>


struct MallocMetadata{
    size_t size;
    bool is_free;
    MallocMetadata* next;
    MallocMetadata* prev;
};

//Global variables:
MallocMetadata* BlockList;
MallocMetadata* BlockListTail;

size_t free_blocks;
size_t free_bytes;
size_t allocated_blocks;
size_t allocated_bytes;

//Functions:
static void* getStart(MallocMetadata* block){
    block++;
}
static size_t getSize(MallocMetadata* block);
static MallocMetadata* findBlock(size_t s){
    MallocMetadata* curr=BlockList;
    while(curr!=NULL && curr->size<s){
        curr=curr->next;
    }
    if(!curr){
        return NULL;
    }
    return curr;
}
static bool UnFree(MallocMetadata* block){
    block->is_free=false;
    free_blocks--;
    free_bytes=free_bytes-block->size;
}



static MallocMetadata* getMetaDataByPointer(void* mem){
    MallocMetadata* ptr= BlockList;
    while(ptr!=NULL){
        if (ptr++==mem)//TODO: ask about ++
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
    allocated_blocks--;
    allocated_bytes-=metadata->size;
    free_blocks++;
    free_bytes+=metadata->size;
}




void* smalloc(size_t size){

    unsigned int s=size; //check conversion
    if (s<0 || size>100000000){
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
