
#include <unistd.h>


struct MallocMetadata{
    size_t size;
    bool is_free;
    MallocMetadata* next;
    MallocMetadata* prev;
};

//Global variables:
MallocMetadata* BlockList;
size_t free_blocks;
size_t free_bytes;
size_t allocated_blocks;
size_t allocated_bytes;

//Functions:
static void* getStart(MallocMetadata* block);
static size_t getSize(MallocMetadata* block);
static MallocMetadata* findBlock(MallocMetadata* bList, size_t s);
static bool UnFree(MallocMetadata* block);


void* smalloc(size_t size){

    unsigned int s=size; //check conversion
    if (s<0 || size>100000000){
        return NULL;
    }
    void* ptr;
    MallocMetadata* block= findBlock(BlockList, size);
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

