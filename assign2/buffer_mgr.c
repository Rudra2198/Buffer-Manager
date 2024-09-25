#include "buffer_mgr.h"
#include "stdlib.h"
#include <unistd.h>
#include <storage_mgr.h>

int writtenToDisk = 0;
int readFromDisk = 0;
int lruCounter = 0;
bool isInitialized_bp;
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
RC initBufferPool(BM_BufferPool *const bm, const char *const pageFileName,
                  const int numPages, ReplacementStrategy strategy,
                  void *stratData) {
    printf("Initializing the Buffer Pool.\n");
    isInitialized_bp = false;
    // Check if the file exists
    if (access(pageFileName, F_OK) != -1) {
        printf("File '%s' exists.\n", pageFileName);
    } else {
        return RC_FILE_NOT_FOUND; // Define appropriate error code
    }

    // Allocate memory for the buffer pool
    bm->mgmtData = malloc(sizeof(Frames) * numPages);
    if (bm->mgmtData == NULL) {
        return RC_BP_INIT_ERROR;
    }

    // Initialize the individual frames in the buffer pool
    Frames *frames = (Frames *)bm->mgmtData;
    for (int i = 0; i < numPages; i++) {
        frames[i].pageNumber = NO_PAGE;
        frames[i].memPage = NULL;
        frames[i].dirty = false;
        frames[i].fix_cnt = 0;
        frames[i].lruOrder = 0;
    }

    int *data = (int *)stratData;
    if (data != NULL) {
        // Use the value of the strategy-specific data
        bm->stratParam = *data;
        // Proceed with initializing the buffer pool using the value
    }

    // Initialize other properties of the buffer pool
    bm->pageFile = (char*)pageFileName;
    bm->numPages = numPages;
    bm->strategy = strategy;

    writtenToDisk = 0;
    readFromDisk = 0;

    printf("Buffer Pool has initialized.\n");
    isInitialized_bp=true;

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
RC shutdownBufferPool(BM_BufferPool *const bm) {
    printf("Shutting down the Buffer Pool.\n");
    if (isInitialized_bp == false) {
        return RC_BP_SHUNTDOWN_ERROR;
    }
    Frames *frames = (Frames *) bm->mgmtData;

    // Write dirty page back to disk
    forceFlushPool(bm);

    // Now free the memory for each page frame
    for (int i = 0; i < bm->numPages; i++) {
        if (frames[i].memPage != NULL) {
            free(frames[i].memPage);
            frames[i].memPage = NULL; // Avoid dangling pointer
        }
    }

    // Free memory associated with the buffer pool
    free(frames);
    bm->mgmtData = NULL;

    printf("Buffer Pool has shut down.\n");
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
RC forceFlushPool(BM_BufferPool *const bm) {
    printf("Forcing flush the Buffer Pool.\n");
    if (isInitialized_bp == false) {
        return RC_BP_FLUSHPOOL_FAILED;
    }
    Frames *frames = (Frames *) bm->mgmtData;
    int numPages = bm->numPages;
    int check_error = 0;

    // Check for pinned pages
    for (int i = 0; i< numPages; i++) {
        if (frames[i].dirty == true && frames[i].fix_cnt == 0) {
            SM_FileHandle fHandle;
            openPageFile(bm->pageFile, &fHandle);
            writeBlock(frames[i].pageNumber, &fHandle, frames[i].memPage);
            closePageFile(&fHandle);
            frames[i].dirty = false;
            writtenToDisk++;
        } else {
            check_error++;
        }
        if (frames[i].fix_cnt != 0) {
            return RC_BP_FLUSHPOOL_FAILED;
        }
    }

    if (check_error == numPages) {
        return RC_BP_FLUSHPOOL_FAILED;
    } else {
        printf("Finished force flush pool.\n");
        return RC_OK;
    }
}