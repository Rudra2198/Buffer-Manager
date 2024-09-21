#include "buffer_mgr.h"
#include "storage_mgr.h"
#include "stdlib.h"
#include "stdbool.h"

// Global variables to track disk operations and buffer manager state
int diskWrites = 0;
int diskReads = 0;
int lruCounter = 0;
bool bufferPoolInitialized = false;

/*
 * Function to initialize a buffer pool.
 *
 * Parameters:
 * - bm: Pointer to the buffer pool to be initialized.
 * - pageFileName: The name of the page file to be managed by the buffer pool.
 * - numPages: The number of pages (frames) in the buffer pool.
 * - strategy: The page replacement strategy.
 * - repstratData: Additional data for the replacement strategy.
 *
 * Returns:
 * - RC_OK on successful initialization, otherwise an error code.
 */
RC initBufferPool(BM_BufferPool *bm, const char *pageFileName, int numPages, ReplacementStrategy strategy, void *repstratData) {
    SM_FileHandle fh;
    
    // Try to open the page file, return an error if it doesn't exist or fails to open
    RC result = openPageFile((char *)pageFileName, &fh);
    if (result != RC_OK) {
        return result;  // Return the error code (e.g., RC_FILE_NOT_FOUND)
    }

    // Allocate memory for managing the buffer pool's pages
    BM_PageHandle *frames = (BM_PageHandle *)calloc(numPages, sizeof(BM_PageHandle));
    if (frames == NULL) {
        closePageFile(&fh);  // Close the file handle before returning
        return RC_MEMORY_ALLOCATION_ERROR;  // Return error if memory allocation fails
    }

    // Initialize each page frame to invalid state (NO_PAGE) and set data pointer to NULL
    for (int i = 0; i < numPages; i++) {
        frames[i].pageNum = NO_PAGE;
        frames[i].data = NULL;
    }

    // Populate the BufferPool structure with the provided and initialized values
    bm->pageFile = (char *)pageFileName;
    bm->numPages = numPages;
    bm->strategy = strategy;
    bm->mgmtData = frames;

    // Optionally handle specific replacement strategy data using repstratData
    // For example, if LRU-k needs to use the additional parameter

    // Mark the buffer pool as initialized globally
    bufferPoolInitialized = true;

    // Close the page file after completing the initialization process
    closePageFile(&fh);

    // Return success code
    return RC_OK;
}
