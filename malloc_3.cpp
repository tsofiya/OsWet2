
#include <unistd.h>
#include <cstring>
#include <sys/mman.h>



struct MallocMetadata{
    size_t size;
    bool is_free;
    MallocMetadata* next;
    MallocMetadata* prev;
};



//Global variables:
MallocMetadata* BlockList=NULL;
MallocMetadata* BlockListTail=NULL; //wilderness
size_t free_blocks=0;
size_t free_bytes=0;
size_t allocated_blocks=0;
size_t allocated_bytes=0;

//Functions:
static void* getStart(MallocMetadata* block){
    block++;
}

static size_t getSize(MallocMetadata* block);
static MallocMetadata* findBlock(size_t s){
    MallocMetadata *ptr= BlockList;
    while (ptr!=NULL){
        if (ptr->is_free && s<=ptr->size){
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
    if (metadata->prev->is_free && metadata->next->is_free){
        metadata->prev->size+=metadata->next->size + metadata->size +2* sizeof(MallocMetadata);
        metadata->prev->next= metadata->next->next;
        metadata->next->next->prev= metadata->prev;
    }else if(metadata->prev->is_free){
        metadata->prev->size+= metadata->size+sizeof(MallocMetadata);
        metadata->prev->next= metadata->next;
        metadata->next->prev= metadata->prev;
    }else if(metadata->next->is_free){
        metadata->size+= metadata->next->size+sizeof(MallocMetadata);
        metadata->next= metadata->next->next;
        metadata->next->prev=metadata;
    }
}

MallocMetadata* split(MallocMetadata* block, size_t size){
    size_t total_size= block->size;
    MallocMetadata* new_block= (MallocMetadata*)(block+sizeof(MallocMetadata)+ size);
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
    size_t addition=s-BlockListTail->size;
    void * ptr= sbrk(addition);
    if((ptr==(void*)(-1))){
        return NULL;
    }

    allocated_bytes+= addition;
    BlockListTail->size=s;
    BlockListTail->is_free= false;
    return BlockListTail;
}

void* smalloc(size_t size){

    unsigned int s=size; //check conversion
    if (s<0 || size>100000000){
        return NULL;
    }
    if (s>=131072){
        unsigned long long* ptr= (unsigned long long*)allocateMmap(s);
        return  ptr++;
    }else {
        void *ptr;
        MallocMetadata *block = findBlock(size);
        if (!block) {
            if (BlockListTail == NULL || BlockListTail->is_free == false) {
                enlargeWilderness(size);
            } else {
                block = allocateMetadataAndMem(size);
            }
        } else {
            if (block->size >= 128 + sizeof(MallocMetadata) + size) {
                block = split(block, size);
            }
        }
        UnFree(block);
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
        ptr--;
        munmap(ptr, *ptr);
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
    MallocMetadata* metadata= findBlock(num*size);
    if (metadata== NULL){
        metadata= allocateMetadataAndMem(num*size);
        if (metadata==NULL)
            return NULL;
    }
    return memset(getStart(metadata), 0, num*size);
}

void* srealloc(void* oldp, size_t size){
    if (oldp==NULL)
        return smalloc(size);
    if (size==0)
        return NULL;

    MallocMetadata* oldMetaData= getMetaDataByPointer(oldp);
    if (oldMetaData->size>=size)
        return oldMetaData++;
    MallocMetadata* newMetaData= findBlock(size);
    if (newMetaData==NULL)
        newMetaData= allocateMetadataAndMem(size);
    memcpy(getStart(newMetaData), getStart(oldMetaData), oldMetaData->size);
    oldMetaData->is_free= true;
    freeMetaData(oldMetaData);
    return newMetaData;
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