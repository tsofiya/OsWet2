//
// Created by student on 12/9/19.
//
#include <unistd.h>

void* smalloc(size_t size){

    if (size<0 || size>100000000){
        return NULL;
    }
    if((sbrk(size))==(void*)(-1)){
        return NULL;
    }
    void* ptr=sbrk(0);
    return ptr;
}


