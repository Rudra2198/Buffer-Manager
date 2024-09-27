#include <stdio.h>
#include "buffer_mgr.h"
#include "stdlib.h"
#include <unistd.h>
#include "dt.h"
#include <storage_mgr.h>

typedef struct Page {
    SM_PageHandle data;
    PageNumber pageNum;
    int dirtyBit;
    int fixCount; 
    int hitNum;
    int refNum;
} PageFrame;

//Variables to work on the buffer functions 
int bufferSize = 0;
int recent_page_index = 0;
int writeCount = 0;
int hit = 0;
int clockPointer = 0;
int lfu_pointer = 0;
//Replacement Stratergies - FIFO (First In First Out)
//LRU (Least Recently Used)

//FIFO:
extern void FIFO(BM_BufferPool *const bm, PageFrame *page) {
    PageFrame *pageFrame = (PageFrame *)bm->mgmtData;
    int i, track_page;
    //track-page is used to track the page at front of FIFO queue
    track_page = recent_page_index % bufferSize;

    for (i = 0; i < bufferSize; i++) {
        //Find the suitable page to replace
        if (pageFrame[track_page].fixCount == 0) { //Check if the track_page is not currently in use
            //Check if the page is modified
            if (pageFrame[track_page].dirtyBit == 1) {
                SM_FileHandle fh;
                openPageFile(bm->pageFile, &fh); //Open the page for writing
                //Writing the contents of the dirty page back at disk
                writeBlock(pageFrame[track_page].pageNum, &fh, pageFrame[track_page].data);

                writeCount++;
            }
            //Replace all the data, pageNum, dirtyBit, fixCount with new page's data
            pageFrame[track_page].data = page->data;
            pageFrame[track_page].pageNum = page->pageNum;
            pageFrame[track_page].dirtyBit = page->dirtyBit;
            pageFrame[track_page].fixCount = page->fixCount;
            break;
        } else {
            track_page++; // Move to the next page buffer
            track_page = (track_page % bufferSize == 0) ? 0 : track_page;
        }
    }
}

//LRU - Least Recently used
extern void LRU(BM_BufferPool *const bm, PageFrame *page) {
    PageFrame *pageFrame = (PageFrame *)bm->mgmtData;
    int i, leastHitIndex, leastHitNum;

    //Looping through all the pages to find the first unpinned page
    for (i = 0; i < bufferSize; i++) {
        if (pageFrame[i].fixCount == 0) {
            leastHitIndex = i;
            //Replace the recently used page with the first unpinned page
            leastHitNum = pageFrame[i].hitNum;
            break;
        }
    }


    for (i = leastHitIndex + 1; i < bufferSize; i++) {
        //Find the page which is least recently used
        if (pageFrame[i].hitNum < leastHitNum) {
            leastHitIndex = i;
            leastHitNum = pageFrame[i].hitNum;
        }
    }

    //If recently used page is dirty then write it to disk
    if (pageFrame[leastHitIndex].dirtyBit == 1) {
        SM_FileHandle fh;
        openPageFile(bm->pageFile, &fh);
        //Write in the page in the chosen pagenumber in the specific pageFrame
        writeBlock(pageFrame[leastHitIndex].pageNum, &fh, pageFrame[leastHitIndex].data);

        writeCount++;
    }

    // Replace the least recently used page with the new page
    pageFrame[leastHitIndex].data = page->data;
    pageFrame[leastHitIndex].pageNum = page->pageNum;
    pageFrame[leastHitIndex].dirtyBit = page->dirtyBit;
    pageFrame[leastHitIndex].fixCount = page->fixCount;
    pageFrame[leastHitIndex].hitNum = page->hitNum;
}


//LFU - Least Frequently used 
extern void LFU(BM_BufferPool *const bm, PageFrame *page) {
    PageFrame *pageFrame = (PageFrame *)bm->mgmtData;
    int i, j, least_freq_index, leastFreqRef;
    //Get the least frequency index from the track position i.e. 
    least_freq_index = lfu_pointer;

    for (i = 0; i < bufferSize; i++) {
        //Find an unpinned page and then update the least_freq_index
        if (pageFrame[least_freq_index].fixCount == 0) {
            least_freq_index = (least_freq_index + i) % bufferSize;
            leastFreqRef = pageFrame[least_freq_index].refNum;
            break;
        }
    }

    i = (least_freq_index + 1) % bufferSize;

    for (j = 0; j < bufferSize; j++) {
        //Find the page with the lowest reference count
        if (pageFrame[i].refNum < leastFreqRef) {
            least_freq_index = i;
            leastFreqRef = pageFrame[i].refNum;
        }
        i = (i + 1) % bufferSize;
    }

    //If page is dirty, write to the disk
    if (pageFrame[least_freq_index].dirtyBit == 1) {
        SM_FileHandle fh;
        openPageFile(bm->pageFile, &fh);
        writeBlock(pageFrame[least_freq_index].pageNum, &fh, pageFrame[least_freq_index].data);

        //Increment as the number of writes to the disk
        writeCount++;
        
    }
    //Replace the least frequently used page with new page
    pageFrame[least_freq_index].data = page->data;
    pageFrame[least_freq_index].pageNum = page->pageNum;
    pageFrame[least_freq_index].dirtyBit = page->dirtyBit;
    pageFrame[least_freq_index].fixCount = page->fixCount;

    //Update the LFU pointer for the next replacement
    lfu_pointer = least_freq_index + 1;
}

extern void CLOCK(BM_BufferPool *const bm, PageFrame *page) {
    PageFrame *pageFrame = (PageFrame *)bm->mgmtData;

    //While loop until an unpinned page is found
    while (1) {
        clockPointer = (clockPointer % bufferSize == 0) ? 0 : clockPointer;

        if (pageFrame[clockPointer].hitNum == 0) {
            //If page is dirty write it to the disk
            if (pageFrame[clockPointer].dirtyBit == 1) {
                SM_FileHandle fh;
                openPageFile(bm->pageFile, &fh);
                writeBlock(pageFrame[clockPointer].pageNum, &fh, pageFrame[clockPointer].data);

                //incrementing write for statistical function
                writeCount++;
            }

            //Replacing the page in buffer with new page
            pageFrame[clockPointer].data = page->data;
            pageFrame[clockPointer].pageNum = page->pageNum;
            pageFrame[clockPointer].dirtyBit = page->dirtyBit;
            pageFrame[clockPointer].fixCount = page->fixCount;
            pageFrame[clockPointer].hitNum = page->hitNum;
            //Increment the clockpointer
            clockPointer++;
            break;
        } else {
            //Resetting the reference to 0
            pageFrame[clockPointer++].hitNum = 0;
        }
    }
}


extern RC initBufferPool(BM_BufferPool *const bm, const char *const pageFileName,const int numPages, ReplacementStrategy strategy,void *stratData) {
    bm->pageFile = (char *)pageFileName;
    //Set the number of pages that buffer pool will manage
    bm->numPages = numPages;
    //Setting the replacement stratergy to stratergy argument
    bm->strategy = strategy;

    PageFrame *page = malloc(sizeof(PageFrame) * numPages);

    //Initialize the buffer size to the
    bufferSize = numPages;
    int i;

    for (i = 0; i < bufferSize; i++) {
        page[i].data = NULL;
        page[i].pageNum = -1;
        page[i].dirtyBit = 0;
        page[i].fixCount = 0;
        page[i].hitNum = 0;
        page[i].refNum = 0;
    }
    //Storing the page frame array to buffer pool's management data
    bm->mgmtData = page;
    writeCount = clockPointer = lfu_pointer = 0;
    return RC_OK;
}

extern RC forceFlushPool(BM_BufferPool *const bm) {
    PageFrame *pageFrame = (PageFrame *)bm->mgmtData;
    int i;

    for (i = 0; i < bufferSize; i++) {
        //If page is not pinned, and dirty, then the paeg file can write dirty page back to disk
        if (pageFrame[i].fixCount == 0 && pageFrame[i].dirtyBit == 1) {
            SM_FileHandle fh;

            //Open the page file to write the dirty page
            openPageFile(bm->pageFile, &fh);

            //Now write the page back to disk
            writeBlock(pageFrame[i].pageNum, &fh, pageFrame[i].data);
            pageFrame[i].dirtyBit = 0;

            //Incrementing the writing count to track the pages written to disk
            writeCount++;
        }
    }
    return RC_OK;
}


extern RC shutdownBufferPool(BM_BufferPool *const bm) {
    PageFrame *pageFrame = (PageFrame *)bm->mgmtData;
    //Call the function to write any dirty pages
    forceFlushPool(bm);
    int i;
   
    for (i = 0; i < bufferSize; i++) {
        if (pageFrame[i].fixCount != 0) {
            // Return an error indicating that pinned pages are still in the buffer
            return RC_PINNED_PAGES_IN_BUFFER;
        }
    }
    //Free the memory allocated for the page frames
    free(pageFrame);
    //Setting the management data of Buffer manager to NULL as it is no longer in use
    bm->mgmtData = NULL;
    return RC_OK;
}

extern RC markDirty(BM_BufferPool *const bm, BM_PageHandle *const page) {

    //Retrieve the page frame array
    PageFrame *pageFrame = (PageFrame *)bm->mgmtData;
    int i;
   
    for (i = 0; i < bufferSize; i++) {
        //Check if the current page number matches that page to be marked dirty
        if (pageFrame[i].pageNum == page->pageNum) {

            // To represent the page has been modified, set the dirty bit to 1
            pageFrame[i].dirtyBit = 1;
            return RC_OK;
        }
    }
    return RC_ERROR;
}

extern RC unpinPage(BM_BufferPool *const bm, BM_PageHandle *const page) {
    PageFrame *pageFrame = (PageFrame *)bm->mgmtData;
    int i;
   
    for (i = 0; i < bufferSize; i++) {
        //Check if the current page number matches the page tp be unpinned
        if (pageFrame[i].pageNum == page->pageNum) {
            //Decrement the fix count to indicate that this page is no longer pinned
            pageFrame[i].fixCount--;
            break;
        }
    }
    return RC_OK;
}

extern RC forcePage(BM_BufferPool *const bm, BM_PageHandle *const page) {
    PageFrame *pageFrame = (PageFrame *)bm->mgmtData;
    int i;

    for (i = 0; i < bufferSize; i++) {
        // Check if current page number matches the page to be forced to disk
        if (pageFrame[i].pageNum == page->pageNum) {
            SM_FileHandle fh; 
            openPageFile(bm->pageFile, &fh); // Open the page file for writing
            
            // Write the current page's data back to disk
            writeBlock(pageFrame[i].pageNum, &fh, pageFrame[i].data);
            
            // Mark the page as clean by resetting the dirty bit
            pageFrame[i].dirtyBit = 0;
            writeCount++; // Increment the write count for statistics
        }
    }
    return RC_OK;
}

extern RC pinPage(BM_BufferPool *const bm, BM_PageHandle *const page,const PageNumber pageNum) {
    PageFrame *pageFrame = (PageFrame *)bm->mgmtData;

    // Check if the first page frame is empty
    if (pageFrame[0].pageNum == -1) {
        SM_FileHandle fh;
        openPageFile(bm->pageFile, &fh);
        pageFrame[0].data = (SM_PageHandle) malloc(PAGE_SIZE);
        ensureCapacity(pageNum, &fh);
        readBlock(pageNum, &fh, pageFrame[0].data);
        pageFrame[0].pageNum = pageNum;
        pageFrame[0].fixCount++;
        recent_page_index = hit = 0;
        pageFrame[0].hitNum = hit;
        pageFrame[0].refNum = 0;
        page->pageNum = pageNum;
        page->data = pageFrame[0].data;
        return RC_OK;
    } else {
        int i;
        bool isBufferFull = true;

        // Check if the requested page is already in the buffer
        for (i = 0; i < bufferSize; i++) {
            if (pageFrame[i].pageNum != -1) {
                if (pageFrame[i].pageNum == pageNum) {
                    pageFrame[i].fixCount++;
                    isBufferFull = false;
                    hit++;

                    // Update hit number based on replacement strategy
                    if (bm->strategy == RS_LRU)
                        pageFrame[i].hitNum = hit;
                    else if (bm->strategy == RS_CLOCK)
                        pageFrame[i].hitNum = 1;
                    else if (bm->strategy == RS_LFU)
                        pageFrame[i].refNum++;

                    page->pageNum = pageNum;
                    page->data = pageFrame[i].data;
                    clockPointer++;
                    break;
                }
            } else {
                // If there is an empty slot, load the page
                SM_FileHandle fh;
                openPageFile(bm->pageFile, &fh);
                pageFrame[i].data = (SM_PageHandle) malloc(PAGE_SIZE);
                readBlock(pageNum, &fh, pageFrame[i].data);
                pageFrame[i].pageNum = pageNum;
                pageFrame[i].fixCount = 1;
                pageFrame[i].refNum = 0;
                recent_page_index++;
                hit++;

                // Update hit number based on replacement strategy
                if (bm->strategy == RS_LRU)
                    pageFrame[i].hitNum = hit;
                else if (bm->strategy == RS_CLOCK)
                    pageFrame[i].hitNum = 1;

                page->pageNum = pageNum;
                page->data = pageFrame[i].data;
                isBufferFull = false;
                break;
            }
        }

        // If the buffer is full, evict a page using the appropriate strategy
        if (isBufferFull == true) {
            PageFrame *newPage = (PageFrame *) malloc(sizeof(PageFrame));
            SM_FileHandle fh;
            openPageFile(bm->pageFile, &fh);
            newPage->data = (SM_PageHandle) malloc(PAGE_SIZE);
            readBlock(pageNum, &fh, newPage->data);
            newPage->pageNum = pageNum;
            newPage->dirtyBit = 0;
            newPage->fixCount = 1;
            newPage->refNum = 0;
            recent_page_index++;
            hit++;

            // Update hit number based on replacement strategy
            if (bm->strategy == RS_LRU)
                newPage->hitNum = hit;
            else if (bm->strategy == RS_CLOCK)
                newPage->hitNum = 1;

            page->pageNum = pageNum;
            page->data = newPage->data;

            // Use the appropriate replacement strategy to evict a page
            switch (bm->strategy) {
                case RS_FIFO:
                    FIFO(bm, newPage);
                    break;
                case RS_LRU:
                    LRU(bm, newPage);
                    break;
                case RS_CLOCK:
                    CLOCK(bm, newPage);
                    break;
                case RS_LFU:
                    LFU(bm, newPage);
                    break;
                case RS_LRU_K:
                    printf("\n LRU-k algorithm not implemented");
                    break;
                default:
                    printf("\nAlgorithm Not Implemented\n");
                    break;
            }
        }
        return RC_OK;
    }
}
