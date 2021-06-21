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


/**
 * updates the relevant reference parameters
 */
void update_params(uint64_t base, uint64_t curr_path, word_t value, int curr_weight, uint64_t i, uint64_t &max_path,
                   int &max_weight, word_t &toEvict, uint64_t &valPointer) {
    if(curr_weight > max_weight){
     max_path = curr_path;
     max_weight = curr_weight;
     toEvict = value; // save frame to evict
     valPointer = base*PAGE_SIZE + i;// save writing address
    }
}

/**
 *
 * @param depth - depth of path
 * @param base - base address
 * @param curr_path - current path
 * @param emptyFrame - boolean indicating frame is empty
 * @param frameToKeep - a frame in the path that we do not want to evict
 * @param max_path - the path with the maximum weight
 * @param max_weight - the weight of the maximum path
 * @param toEvict - the item we need to remove
 * @param valPointer - a pointer to the item we need to remove
 * @return a frame to be evicted
 */
int dfs(uint64_t depth, uint64_t base, uint64_t curr_path, bool *emptyFrame, uint64_t frameToKeep, uint64_t &max_path,
        int &max_weight, word_t& toEvict, uint64_t& valPointer) {
    word_t value;
    int curr_weight = 0;
    int dfsRet; int foundFrame =0;
    bool valueIsZero = true;
    if(depth == TABLES_DEPTH) { // recursion base
        return 0;
    }
    for (uint64_t i=0;i<PAGE_SIZE;i++){
        PMread(i+base*PAGE_SIZE,&value); // read content of frame
        if(value!=0){ // frame isn't empty
           if(value%2==0){curr_weight += WEIGHT_EVEN;} else{curr_weight += WEIGHT_ODD;}
           uint64_t newPath = (curr_path << OFFSET_WIDTH) +i; // path to the current frame
           valueIsZero = false; // frame isn't empty boolean indicator
           if(depth == TABLES_DEPTH -1){ // find path weights here
               update_params(base, curr_path, value, curr_weight, i, max_path, max_weight, toEvict, valPointer);
           }
           dfsRet = dfs(depth+1, base, newPath, emptyFrame, frameToKeep,
                        max_path, max_weight, toEvict, valPointer); // recursive call for depth+1
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
uint64_t findAddressToEvict(uint64_t frameToKeep){
    bool emptyFrame = false;
    uint64_t maxPath = 0; int maxWeight = 0; word_t evict_init = 0; uint64_t valPointer = 0;
    uint64_t toEvict = dfs(0,0,0,&emptyFrame, frameToKeep, maxPath, maxWeight, evict_init, valPointer);
    uint64_t frameIndex = 0;uint64_t evictedPageIndex = 0;uint64_t physicalAddress = 0;
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
uint64_t handleZeroAddress(int i, uint64_t p_i, word_t &addr1, uint64_t &baseAddress, uint64_t& frameToKeep) {
    uint64_t evicted = findAddressToEvict(frameToKeep);
    if(i==TABLES_DEPTH-1){ //reached tree leaves - can restore evicted
        PMrestore(baseAddress+p_i, evicted);
    }
    else{
        clearTable(evicted);
    }
    PMwrite(baseAddress+p_i, evicted);
    addr1 = evicted;
    return evicted;
}

/**
 * converts a virtual address to physical
 * @param virtualAddress
 * @return
 */
uint64_t toPhysicalAddress(uint64_t virtualAddress){
    uint64_t frameToKeep = 0;
    uint64_t offset = virtualAddress % PAGE_SIZE;
    word_t addr1 = 0;
    uint64_t baseAddress = 0;
    for(int i =0; i<TABLES_DEPTH;i++){
        uint64_t p_i = virtualAddress >> (TABLES_DEPTH-i)* OFFSET_WIDTH;
        p_i = p_i % PAGE_SIZE;
        PMread(baseAddress+p_i,&addr1);
        if(addr1 == 0){
            handleZeroAddress(i, p_i, addr1, baseAddress, frameToKeep);
        }
        frameToKeep = addr1;
        baseAddress = PAGE_SIZE* addr1;
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



uint64_t DFS(uint64_t forbidden, uint64_t baseFrame, int depth, bool *empty, uint64_t * maxFrame,
             uint64_t pageIndex, uint64_t route, uint64_t * farFrame, uint64_t * farFrameDist,
             uint64_t * farFramePointer, uint64_t * farFramePageIndex) {
    *maxFrame = (*maxFrame > baseFrame) ? *maxFrame : baseFrame;
    if (depth == TABLES_DEPTH) {
        return 0;
    }
    uint64_t result=0, dfsResult;
    bool isEmpty = true;
    word_t value;
    for (uint64_t i = 0; i < PAGE_SIZE; ++i) {
        PMread(baseFrame * PAGE_SIZE + i, &value);
        if (value != 0) {
            isEmpty = false;
            uint64_t newRoute = (route << OFFSET_WIDTH) + i;
            if (depth == TABLES_DEPTH - 1) {
                uint64_t absDist = (pageIndex > newRoute) ? (pageIndex - newRoute) :
                                   (newRoute-pageIndex);
                uint64_t dist = (NUM_PAGES - absDist > absDist) ? absDist : NUM_PAGES - absDist;
                *farFrameDist = (*farFrameDist > dist) ? *farFrameDist : dist;
                *farFramePointer = baseFrame * PAGE_SIZE + i;
                *farFramePageIndex = newRoute;
                *farFrame = value;
            }
            dfsResult = DFS(forbidden, value, depth + 1, empty, maxFrame, pageIndex,
                            newRoute, farFrame, farFrameDist, farFramePointer, farFramePageIndex);
            if (!result && dfsResult) {
                if (*empty) {
                    *empty = false;
                    PMwrite(baseFrame * PAGE_SIZE + i, 0);
                }
                result = dfsResult;
            }
        }
    }
    if (isEmpty &&  ((baseFrame != 0) && (baseFrame != forbidden))) {
        *empty = true;
        return baseFrame;
    }
    return result;
}