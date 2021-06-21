#include "VirtualMemory.h"
#include "PhysicalMemory.h"

void clearTable(uint64_t frameIndex) {
    for (uint64_t i = 0; i < PAGE_SIZE; ++i) {
        PMwrite(frameIndex * PAGE_SIZE + i, 0);
    }
}

void VMinitialize() {
    clearTable(0);
}


int dfs(uint64_t depth, uint64_t base, uint64_t path,bool *emptyFrame, uint64_t frameToKeep) {
    word_t value;
    int dfsRet; int foundFrame =0;
    bool valueIsZero = true;
    if(depth == TABLES_DEPTH) { // recursion base
        return 0;
    }
    for (uint64_t i=0;i<PAGE_SIZE;i++){
        PMread(i+base*PAGE_SIZE,&value); // read content of frame
        if(value!=0){ // frame isn't empty
               uint64_t newPath = (path << OFFSET_WIDTH) +i; // path to the current frame
               valueIsZero = false; // frame isn't empty boolean indicator
               if(depth == TABLES_DEPTH -1){ // find path weights here



               }
               dfsRet = dfs(depth+1,value,newPath,emptyFrame,frameToKeep); // recursive call for depth+1
               if(dfsRet!=0 && !foundFrame){ // dfs has found some non empty frame AND didnt found a frame till now
                   if(*emptyFrame){ // the CHILD frame is currently empty - we can write to it
                       PMwrite(base*PAGE_SIZE+i,0); // writing to the parent 0 (unlink from parent)
                       *emptyFrame = false;
                   }
                   foundFrame = dfsRet; // updating that we have found a frame
               }
        }
    }
    if(base!= frameToKeep && base != 0 && valueIsZero){ //making sure to not remove a frame from the path
        *emptyFrame = true;
        return base;
    }
    return foundFrame;
}

/**
 * finds an adress to evict by splitting to various cases
 * @return
 */
uint64_t findAddressToEvict(){
    uint64_t toEvict = dfs();
    uint64_t frameIndex = 0;
    uint64_t evictedPageIndex = 0;
    uint64_t physicalAddress = 0;
    bool freeFrame = frameIndex + 1 < NUM_FRAMES;
    switch(freeFrame){
        case true: // haven't reached num_frames - there exists an unused frame
            toEvict = frameIndex+1;
            break;
        case false: // no free frame exists, must evict
            PMevict(toEvict,evictedPageIndex); //TODO: need to update toEvict
            PMwrite(physicalAddress,0);
            break;
    }
    // found a frame containing an empty table:
    return toEvict;
}


/**
 * this function handles the case where addr1=0
 * @param i
 * @param p_i
 * @param addr1
 * @param baseAddress
 */
void handleZeroAddress(int i, uint64_t p_i, word_t &addr1, uint64_t &baseAddress) {
    uint64_t evicted = findAddressToEvict();
    if(i==TABLES_DEPTH-1){ //reached tree leaves - can restore evicted
        PMrestore(baseAddress+p_i, evicted);
    }
    else{
        clearTable(evicted);
    }
    PMwrite(baseAddress+p_i, evicted);
    addr1 = evicted;
    baseAddress = PAGE_SIZE* addr1;
}

/**
 * converts a virtual address to physical
 * @param virtualAddress
 * @return
 */
uint64_t toPhysicalAddress(uint64_t virtualAddress){
    uint64_t offset = virtualAddress % PAGE_SIZE;
    word_t addr1 = 0;
    uint64_t baseAddress = 0;
    for(int i =0; i<TABLES_DEPTH;i++){
        uint64_t p_i = virtualAddress >> (TABLES_DEPTH-i)* OFFSET_WIDTH;
        p_i = p_i % PAGE_SIZE;
        PMread(baseAddress+p_i,&addr1);
        if(addr1 == 0){
            handleZeroAddress(i, p_i, addr1, baseAddress);
        }
    }
    return addr1 * PAGE_SIZE + offset;


}


int VMread(uint64_t virtualAddress, word_t* value) {
    if (virtualAddress>=VIRTUAL_MEMORY_SIZE){
        return 0;
    }
    uint64_t physicalAddress = toPhysicalAddress(virtualAddress);
    PMread(physicalAddress,value);
    return 1;
}


int VMwrite(uint64_t virtualAddress, word_t value) {
    if (virtualAddress>=VIRTUAL_MEMORY_SIZE){
        return 0;
    }
    uint64_t physicalAddress = toPhysicalAddress(virtualAddress);
    PMwrite(physicalAddress,value);
    return 1;
}
