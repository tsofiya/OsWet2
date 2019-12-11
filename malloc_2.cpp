
#include <unistd.h>
#include <cstring>


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
    return block++;
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
static bool UnFree(MallocMetadata* block);

static MallocMetadata* getMetaDataByPointer(void* mem){
    MallocMetadata* ptr= BlockList;
    while(ptr!=NULL){
        if (ptr++==mem)
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

void freeMetaData(MallocMetadata*metadata){
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
        ptr=sbrk(size);
        if((ptr==(void*)(-1))){
            return NULL;
        }
        ptr=sbrk(0);
        //TODO: UPDATE METADATA!!!
        allocated_blocks++;
        allocated_bytes+= s;

        MallocMetadata* curr =BlockList;
        while(curr->next){
            curr=curr->next;
        }
        curr->next=(MallocMetadata*)ptr; //TODO: what about metadata???

    }
    else{
        UnFree(block);
        ptr=getStart(block);
        free_blocks--;
        free_bytes=-getSize(block);
        return ptr;
    }

}


void sfree (void* p){
    if(!p){
        return;
    }
    unsigned long eff_p=(unsigned long)p-sizeof(MallocMetadata); //TODO: unsigned long?



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
    return sizeof(MallocMetadata)*allocated_blocks;
}