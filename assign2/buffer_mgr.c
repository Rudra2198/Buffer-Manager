#include <stdio.h>
#include <stdlib.h>
#include "buffer_mgr.h"
#include "storage_mgr.h"
#include <math.h>

typedef struct Page {
    SM_PageHandle data;
    PageNumber pageNum;
    int dirtyBit;
    int fixCount;
    int hitNum;
    int refNum;
} PageFrame;

//Variables to work on the buffer functions 
int buffer_size = 0;
int last_index_bp = 0; //Last position in buffer pool used for FIFO Stratergy
int track_write_count = 0;
int hit = 0;  //Count the page hits - when page is already present in the buffer
int clock_pointer = 0;
int lfu_pointer = 0;

// Function prototypes
extern void FIFO(BM_BufferPool *const bm, PageFrame *page);
extern void LFU(BM_BufferPool *const bm, PageFrame *page);
extern void LRU(BM_BufferPool *const bm, PageFrame *page);
extern void CLOCK(BM_BufferPool *const bm, PageFrame *page);

extern RC initBufferPool(BM_BufferPool *const bm, const char *const pageFileName,const int numPages, ReplacementStrategy strategy, void *stratData);

extern RC shutdownBufferPool(BM_BufferPool *const bm);
extern RC forceFlushPool(BM_BufferPool *const bm);
extern RC markDirty(BM_BufferPool *const bm, BM_PageHandle *const page);
extern RC unpinPage(BM_BufferPool *const bm, BM_PageHandle *const page);
extern RC forcePage(BM_BufferPool *const bm, BM_PageHandle *const page);
extern RC pinPage(BM_BufferPool *const bm, BM_PageHandle *const page,
                   const PageNumber pageNum);

extern PageNumber *getFrameContents(BM_BufferPool *const bm);
extern bool *getDirtyFlags(BM_BufferPool *const bm);
extern int *getFixCounts(BM_BufferPool *const bm);
extern int getNumReadIO(BM_BufferPool *const bm);
extern int getNumWriteIO(BM_BufferPool *const bm);

//Replacement Stratergies - FIFO (First In First Out)

//FIFO:
// Replaces the oldest page in the buffer, following a queue-like structure.
// The page loaded first is the first removed from the buffer.
extern void FIFO(BM_BufferPool *const bm, PageFrame *page) {
    PageFrame *pageFrame = (PageFrame *)bm->mgmtData;
    int i, from_index_FIFO;
    //track-page is used to track the page at front of FIFO queue

    from_index_FIFO = last_index_bp % buffer_size;

    for (i = 0; i < buffer_size; i++) {
        //Find the suitable page to replace
        if (pageFrame[from_index_FIFO].fixCount == 0) { //Check if the track_page is not currently in use
            //Check if the page is modified
            if (pageFrame[from_index_FIFO].dirtyBit == 1) {
                SM_FileHandle fh;
                openPageFile(bm->pageFile, &fh); //Open the page for writing
                //Writing the contents of the dirty page back at disk
                writeBlock(pageFrame[from_index_FIFO].pageNum, &fh, pageFrame[from_index_FIFO].data);

                track_write_count++;
            }
            
            //Replace all the data, pageNum, dirtyBit, fixCount with new page's data
            pageFrame[from_index_FIFO].data = page->data;
            pageFrame[from_index_FIFO].pageNum = page->pageNum;
            pageFrame[from_index_FIFO].dirtyBit = page->dirtyBit;
            pageFrame[from_index_FIFO].fixCount = page->fixCount;
            break;
        } else {
            from_index_FIFO++;  // Move to the next page buffer
            from_index_FIFO = (from_index_FIFO % buffer_size == 0) ? 0 : from_index_FIFO;
        }
    }
}

//LFU - Least Frequently used 
// Replaces the page with the lowest access frequency over time.
// Gives more priority to pages that have been accessed the least number of times.
extern void LFU(BM_BufferPool *const bm, PageFrame *page) {
    PageFrame *pageFrame = (PageFrame *)bm->mgmtData;
    int i, j, least_freq_index, lf_ref_pointer;
    //Get the least frequency index from the track position  
    least_freq_index = lfu_pointer;

    for (i = 0; i < buffer_size; i++) {
        //Find an unpinned page and then update the least_freq_index
        if (pageFrame[least_freq_index].fixCount == 0) {
            least_freq_index = (least_freq_index + i) % buffer_size;
            lf_ref_pointer = pageFrame[least_freq_index].refNum;
            break;
        }
    }

    i = (least_freq_index + 1) % buffer_size;

    for (j = 0; j < buffer_size; j++) {
        //Find the page with the lowest reference count
        if (pageFrame[i].refNum < lf_ref_pointer) {
            least_freq_index = i;
            lf_ref_pointer = pageFrame[i].refNum;
        }
        i = (i + 1) % buffer_size;
    }
    //If page is dirty, write to the disk
    if (pageFrame[least_freq_index].dirtyBit == 1) {
        SM_FileHandle fh;
        openPageFile(bm->pageFile, &fh);
        writeBlock(pageFrame[least_freq_index].pageNum, &fh, pageFrame[least_freq_index].data);

        //Increment as the number of writes to the disk
        track_write_count++;
    }




    //Replace the least frequently used page with new page
    pageFrame[least_freq_index].data = page->data;
    pageFrame[least_freq_index].pageNum = page->pageNum;
    pageFrame[least_freq_index].dirtyBit = page->dirtyBit;
    pageFrame[least_freq_index].fixCount = page->fixCount;
    //Update the LFU pointer for the next replacement
    lfu_pointer = least_freq_index + 1;
}

//LRU - Least Recently used
// Replaces the least recently used page, prioritizing frequently accessed pages.
// Tracks page access history to identify the least recently accessed page.
extern void LRU(BM_BufferPool *const bm, PageFrame *page) {
    PageFrame *pageFrame = (PageFrame *)bm->mgmtData;
    int i, least_hit_index = 0, least_hit_num;

    //Looping through all the pages to find the first unpinned page
    for (i = 0; i < buffer_size; i++) {
        if (pageFrame[i].fixCount == 0) {
            least_hit_index = i;
            //Replace the recently used page with the first unpinned page
            least_hit_num = pageFrame[i].hitNum;
            break;
        }
    }

    for (i = least_hit_index + 1; i < buffer_size; i++) {
        //Find the page which is least recently used
        if (pageFrame[i].hitNum < least_hit_num) {
            least_hit_index = i;
            least_hit_num = pageFrame[i].hitNum;
        }
    }

    //If recently used page is dirty then write it to disk
    if (pageFrame[least_hit_index].dirtyBit == 1) {
        SM_FileHandle fh;
        openPageFile(bm->pageFile, &fh);
        //Write in the page in the chosen pagenumber in the specific pageFrame
        writeBlock(pageFrame[least_hit_index].pageNum, &fh, pageFrame[least_hit_index].data);

        track_write_count++;
    }

    // Replace the least recently used page with the new page
    pageFrame[least_hit_index].data = page->data;

    pageFrame[least_hit_index].pageNum = page->pageNum;

    pageFrame[least_hit_index].dirtyBit = page->dirtyBit;

    pageFrame[least_hit_index].fixCount = page->fixCount;
    
    pageFrame[least_hit_index].hitNum = page->hitNum;
}

//CLOCK Replacement stratergy
// Uses a circular buffer to give pages a second chance before eviction.
// Pages with a "use" bit set to 1 get a second chance, while 0 are replaced.

extern void CLOCK(BM_BufferPool *const bm, PageFrame *page) {
    PageFrame *pageFrame = (PageFrame *)bm->mgmtData;
    //While loop until an unpinned page is found
    while (1) {
        clock_pointer = (clock_pointer % buffer_size == 0) ? 0 : clock_pointer;

        if (pageFrame[clock_pointer].hitNum == 0) {
            //If page is dirty write it to the disk
            if (pageFrame[clock_pointer].dirtyBit == 1) {
                SM_FileHandle fh;
                openPageFile(bm->pageFile, &fh);
                writeBlock(pageFrame[clock_pointer].pageNum, &fh, pageFrame[clock_pointer].data);

                //incrementing write for statistical function
                track_write_count++;
            }

            //Replacing the page in buffer with new page
            pageFrame[clock_pointer].data = page->data;
            pageFrame[clock_pointer].pageNum = page->pageNum;
            pageFrame[clock_pointer].dirtyBit = page->dirtyBit;
            pageFrame[clock_pointer].fixCount = page->fixCount;
            pageFrame[clock_pointer].hitNum = page->hitNum;
            //Increment the clock_pointer
            clock_pointer++;
            break;
        } else {
            //Resetting the reference to 0
            pageFrame[clock_pointer++].hitNum = 0;
        }
    }
}

// Work done by Rudra Patel A20594446

/*
 * Initializes a buffer pool with the specified parameters.
 *
 * Parameters:
 * - bm: Pointer to the buffer pool structure to be initialized.
 * - pageFileName: Name of the page file associated with the buffer pool.
 * - numPages: Number of page frames in the buffer pool.
 * - strategy: Replacement strategy to be used by the buffer pool.
 * - stratData: Additional parameters for the replacement strategy.
 *
 * Returns:
 * - RC_OK if the buffer pool is successfully initialized, otherwise an error code.
 */
extern RC initBufferPool(BM_BufferPool *const bm, const char *const pageFileName, const int numPages, ReplacementStrategy strategy,void *stratData) {
    bm->pageFile = (char *)pageFileName;
    //Set the number of pages that buffer pool will manage
    bm->numPages = numPages;
    //Setting the replacement stratergy to stratergy argument
    bm->strategy = strategy;

    PageFrame *page = malloc(sizeof(PageFrame) * numPages);

    //Initialize the buffer size to the number of pages
    buffer_size = numPages;
    int i;

    for (i = 0; i < buffer_size; i++) {
        page[i].data = NULL;
        page[i].pageNum = -1;
        page[i].dirtyBit = 0;
        page[i].fixCount = 0;
        page[i].hitNum = 0;
        page[i].refNum = 0;
    }

    //Storing the page frame array to buffer pool's management data
    bm->mgmtData = page;
    track_write_count = clock_pointer = lfu_pointer = 0;
    return RC_OK;
}

/*
 * Writes all dirty pages with a fix count of 0 from the buffer pool to disk.
 *
 * Parameters:
 * - bm: Pointer to the buffer pool structure.
 *
 * Returns:
 * - RC_OK if all dirty pages are successfully written to disk, otherwise an error code.
 */
extern RC forceFlushPool(BM_BufferPool *const bm) {
    PageFrame *pageFrame = (PageFrame *)bm->mgmtData;
    int i;

    for (i = 0; i < buffer_size; i++) {
        //If page is not pinned, and dirty, then the paeg file can write dirty page back to disk
        if (pageFrame[i].fixCount == 0 && pageFrame[i].dirtyBit == 1) {
            SM_FileHandle fh;

            //Open the page file to write the dirty page
            openPageFile(bm->pageFile, &fh);

            //Now write the page back to disk
            writeBlock(pageFrame[i].pageNum, &fh, pageFrame[i].data);
            pageFrame[i].dirtyBit = 0;

            //Incrementing the writing count to track the pages written to disk
            track_write_count++;
        }
    }
    return RC_OK;
}

/*
 * Destroys a buffer pool, freeing up all associated resources.
 * If the buffer pool contains any dirty pages with a fix count of 0,
 * they are written back to disk before destroying the pool.
 *
 * Parameters:
 * - bm: Pointer to the buffer pool structure to be shut down.
 *
 * Returns:
 * - RC_OK if the buffer pool is successfully shut down, otherwise an error code.
 */
extern RC shutdownBufferPool(BM_BufferPool *const bm) {
    PageFrame *pageFrame = (PageFrame *)bm->mgmtData;
    //Call the function to write any dirty pages
    forceFlushPool(bm);
    int i;
   
    for (i = 0; i < buffer_size; i++) {
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

//The markDirty function traverses the buffer pool to find the page with a matching page number
extern RC markDirty(BM_BufferPool *const bm, BM_PageHandle *const page) {
    //Retrieve the page frame array
    PageFrame *pageFrame = (PageFrame *)bm->mgmtData;
    int i;
   
    for (i = 0; i < buffer_size; i++) {
        //Check if the current page number matches that page to be marked dirty
        if (pageFrame[i].pageNum == page->pageNum) {
            // To represent the page has been modified, set the dirty bit to 1
            pageFrame[i].dirtyBit = 1;
            return RC_OK;
        }
    }
    return RC_ERROR;
}

// unpinpage function decreases the fix count of the page once it's no longer needed 
// This allows the page to be considered for replacement when the fix count reaches zero.
extern RC unpinPage(BM_BufferPool *const bm, BM_PageHandle *const page) {
    PageFrame *pageFrame = (PageFrame *)bm->mgmtData;
    int i;
   
    for (i = 0; i < buffer_size; i++) {
        //Check if the current page number matches the page tp be unpinned
        if (pageFrame[i].pageNum == page->pageNum) {
            //Decrement the fix count to indicate that this page is no longer pinned
            pageFrame[i].fixCount--;
            break;
        }
    }
    return RC_OK;
}

// ForcePage function writes a specific page into memory
// it ensures the data in the buffer pool for the page is saved to disk.
extern RC forcePage(BM_BufferPool *const bm, BM_PageHandle *const page) {
    PageFrame *pageFrame = (PageFrame *)bm->mgmtData;
    int i;

    for (i = 0; i < buffer_size; i++) {
        // Check if current page number matches the page to be forced to disk
        if (pageFrame[i].pageNum == page->pageNum) {
            SM_FileHandle fh;
            openPageFile(bm->pageFile, &fh); // Open the page file for writing
                        
            // Write the current page's data back to disk
            writeBlock(pageFrame[i].pageNum, &fh, pageFrame[i].data);
            
            // Mark the page as clean by resetting the dirty bit
            pageFrame[i].dirtyBit = 0;
            track_write_count++;  // Increment the write count for statistics
        }
    }
    return RC_OK;
}

//pinPage function pins a page with the given page number into the buffer pool
extern RC pinPage(BM_BufferPool *const bm, BM_PageHandle *const page, const PageNumber pageNum) {
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
        last_index_bp = hit = 0;
        pageFrame[0].hitNum = hit;
        pageFrame[0].refNum = 0;
        page->pageNum = pageNum;
        page->data = pageFrame[0].data;
        return RC_OK;
    } else {
        int i;
        bool isBufferFull = true;
       
       // Check if the requested page is already in the buffer
        for (i = 0; i < buffer_size; i++) {
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
                    clock_pointer++;
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
                last_index_bp++;
                hit++;

                // Update hit number based on replacement strategy between LRU and CLOCK
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
            last_index_bp++;
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


/********************* Statistics Functions*****************************/
//All the statistical functions will track the information about the buffer pool and its usage

//1. getFrameContents is rerpsonsible for returning an array of page numbers stored in the buffer
// pool's page frames.
extern PageNumber *getFrameContents(BM_BufferPool *const bm) {
    //Allocate memory for the array of PageNumbers
    PageNumber *frameContents = malloc(sizeof(PageNumber) * buffer_size);
    PageFrame *pageFrame = (PageFrame *) bm->mgmtData;
    int i = 0;
   
   //Memory Allocation error
    if(frameContents == NULL){
        return NULL;
    }
    //to retrieve the page numbers, we can loop through each frame in pool
    while (i < buffer_size) {
        frameContents[i] = (pageFrame[i].pageNum != -1) ? pageFrame[i].pageNum : NO_PAGE;
        i++;
    }
    return frameContents;
}

//getDirtyFlags is a function used to create an array of boolean values whose page is "dirty" in a page frame
extern bool *getDirtyFlags(BM_BufferPool *const bm) {
    //Allocate some memory for the array of dirty flags
    //Calculating the total amount of memory needed for the array
    bool *dirtyFlags = malloc(sizeof(bool) * buffer_size);

    // Access the frames in the buffer pool's management data
    PageFrame *pageFrame = (PageFrame *)bm->mgmtData;
    int i;

    if (dirtyFlags == NULL) {
        // Handle memory allocation error
        return NULL;
    }

    // Iterate through the buffer pool pages to determine the dirty status
    for (i = 0; i < buffer_size; i++) {
        dirtyFlags[i] = (pageFrame[i].dirtyBit == 1) ? true : false ;
    }

    return dirtyFlags;   // Return the array of dirty flags
}

//getFixCounts will shows how many clients are currently using the page
//It represents the fix count for each page
extern int *getFixCounts(BM_BufferPool *const bm) {
    // Allocating memory for the array of fix counts
    int *fixCounts = malloc(sizeof(int) * buffer_size);

    // Access the frames in the buffer pool's management data
    PageFrame *pageFrame= (PageFrame *)bm->mgmtData;
    int i = 0;

    if (fixCounts == NULL) {
        return NULL; // Memory allocation error
    }
    
    // Iterate through the buffer pool pages to retrieve the fix counts
    while (i < buffer_size) {
        fixCounts[i] = (pageFrame[i].fixCount != -1) ? pageFrame[i].fixCount : 0; // Get the fix count for each page
        i++;
    }
    // Return the array of fix counts
    return fixCounts;
}

//getNUmReadIO is used to return the number of times a page was read
//This is helpful to choose the replacement stratergy : LRU
extern int getNumReadIO(BM_BufferPool *const bm) {
    return (last_index_bp + 1);
}

//getNumWriteIO is a function used to count the number of times a page is written on to the disk
extern int getNumWriteIO(BM_BufferPool *const bm) {
    return track_write_count;
}











