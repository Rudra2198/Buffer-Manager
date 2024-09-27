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

int bufferSize = 0;
int rearIndex = 0;
int writeCount = 0;
int hit = 0;
int clockPointer = 0;
int lfuPointer = 0;

// Function prototypes
extern void FIFO(BM_BufferPool *const bm, PageFrame *page);
extern void LFU(BM_BufferPool *const bm, PageFrame *page);
extern void LRU(BM_BufferPool *const bm, PageFrame *page);
extern void CLOCK(BM_BufferPool *const bm, PageFrame *page);

extern RC initBufferPool(BM_BufferPool *const bm, const char *const pageFileName,
                         const int numPages, ReplacementStrategy strategy,
                         void *stratData);

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

// Function implementations

extern void FIFO(BM_BufferPool *const bm, PageFrame *page) {
    PageFrame *pageFrame = (PageFrame *)bm->mgmtData;
    int i, frontIndex;
    frontIndex = rearIndex % bufferSize;

    for (i = 0; i < bufferSize; i++) {
        if (pageFrame[frontIndex].fixCount == 0) {
            if (pageFrame[frontIndex].dirtyBit == 1) {
                SM_FileHandle fh;
                openPageFile(bm->pageFile, &fh);
                writeBlock(pageFrame[frontIndex].pageNum, &fh, pageFrame[frontIndex].data);

                writeCount++;
            }

            pageFrame[frontIndex].data = page->data;
            pageFrame[frontIndex].pageNum = page->pageNum;
            pageFrame[frontIndex].dirtyBit = page->dirtyBit;
            pageFrame[frontIndex].fixCount = page->fixCount;
            break;
        } else {
            frontIndex++;
            frontIndex = (frontIndex % bufferSize == 0) ? 0 : frontIndex;
        }
    }
}

extern void LFU(BM_BufferPool *const bm, PageFrame *page) {
    PageFrame *pageFrame = (PageFrame *)bm->mgmtData;
    int i, j, leastFreqIndex, leastFreqRef;
    leastFreqIndex = lfuPointer;

    for (i = 0; i < bufferSize; i++) {
        if (pageFrame[leastFreqIndex].fixCount == 0) {
            leastFreqIndex = (leastFreqIndex + i) % bufferSize;
            leastFreqRef = pageFrame[leastFreqIndex].refNum;
            break;
        }
    }

    i = (leastFreqIndex + 1) % bufferSize;

    for (j = 0; j < bufferSize; j++) {
        if (pageFrame[i].refNum < leastFreqRef) {
            leastFreqIndex = i;
            leastFreqRef = pageFrame[i].refNum;
        }
        i = (i + 1) % bufferSize;
    }

    if (pageFrame[leastFreqIndex].dirtyBit == 1) {
        SM_FileHandle fh;
        openPageFile(bm->pageFile, &fh);
        writeBlock(pageFrame[leastFreqIndex].pageNum, &fh, pageFrame[leastFreqIndex].data);

        writeCount++;
    }

    pageFrame[leastFreqIndex].data = page->data;
    pageFrame[leastFreqIndex].pageNum = page->pageNum;
    pageFrame[leastFreqIndex].dirtyBit = page->dirtyBit;
    pageFrame[leastFreqIndex].fixCount = page->fixCount;
    lfuPointer = leastFreqIndex + 1;
}

extern void LRU(BM_BufferPool *const bm, PageFrame *page) {
    PageFrame *pageFrame = (PageFrame *)bm->mgmtData;
    int i, leastHitIndex, leastHitNum;

    for (i = 0; i < bufferSize; i++) {
        if (pageFrame[i].fixCount == 0) {
            leastHitIndex = i;
            leastHitNum = pageFrame[i].hitNum;
            break;
        }
    }

    for (i = leastHitIndex + 1; i < bufferSize; i++) {
        if (pageFrame[i].hitNum < leastHitNum) {
            leastHitIndex = i;
            leastHitNum = pageFrame[i].hitNum;
        }
    }

    if (pageFrame[leastHitIndex].dirtyBit == 1) {
        SM_FileHandle fh;
        openPageFile(bm->pageFile, &fh);
        writeBlock(pageFrame[leastHitIndex].pageNum, &fh, pageFrame[leastHitIndex].data);

        writeCount++;
    }

    pageFrame[leastHitIndex].data = page->data;
    pageFrame[leastHitIndex].pageNum = page->pageNum;
    pageFrame[leastHitIndex].dirtyBit = page->dirtyBit;
    pageFrame[leastHitIndex].fixCount = page->fixCount;
    pageFrame[leastHitIndex].hitNum = page->hitNum;
}





extern void CLOCK(BM_BufferPool *const bm, PageFrame *page) {
    PageFrame *pageFrame = (PageFrame *)bm->mgmtData;
    while (1) {
        clockPointer = (clockPointer % bufferSize == 0) ? 0 : clockPointer;

        if (pageFrame[clockPointer].hitNum == 0) {
            if (pageFrame[clockPointer].dirtyBit == 1) {
                SM_FileHandle fh;
                openPageFile(bm->pageFile, &fh);
                writeBlock(pageFrame[clockPointer].pageNum, &fh, pageFrame[clockPointer].data);

                writeCount++;
            }

            pageFrame[clockPointer].data = page->data;
            pageFrame[clockPointer].pageNum = page->pageNum;
            pageFrame[clockPointer].dirtyBit = page->dirtyBit;
            pageFrame[clockPointer].fixCount = page->fixCount;
            pageFrame[clockPointer].hitNum = page->hitNum;
            clockPointer++;
            break;
        } else {
            pageFrame[clockPointer++].hitNum = 0;
        }
    }
}

// Work done by Rudra Patel A20594446
extern RC initBufferPool(BM_BufferPool *const bm, const char *const pageFileName,
                         const int numPages, ReplacementStrategy strategy,
                         void *stratData) {
    bm->pageFile = (char *)pageFileName;
    bm->numPages = numPages;
    bm->strategy = strategy;

    PageFrame *page = malloc(sizeof(PageFrame) * numPages);

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

    bm->mgmtData = page;
    writeCount = clockPointer = lfuPointer = 0;
    return RC_OK;
}

extern RC forceFlushPool(BM_BufferPool *const bm) {
    PageFrame *pageFrame = (PageFrame *)bm->mgmtData;
    int i;

    for (i = 0; i < bufferSize; i++) {
        if (pageFrame[i].fixCount == 0 && pageFrame[i].dirtyBit == 1) {
            SM_FileHandle fh;
            openPageFile(bm->pageFile, &fh);
            writeBlock(pageFrame[i].pageNum, &fh, pageFrame[i].data);
            pageFrame[i].dirtyBit = 0;
            writeCount++;
        }
    }
    return RC_OK;
}

extern RC shutdownBufferPool(BM_BufferPool *const bm) {
    PageFrame *pageFrame = (PageFrame *)bm->mgmtData;
    forceFlushPool(bm);
    int i;
   
    for (i = 0; i < bufferSize; i++) {
        if (pageFrame[i].fixCount != 0) {
            return RC_PINNED_PAGES_IN_BUFFER;
        }
    }

    free(pageFrame);
    bm->mgmtData = NULL;
    return RC_OK;
}


extern RC markDirty(BM_BufferPool *const bm, BM_PageHandle *const page) {
    PageFrame *pageFrame = (PageFrame *)bm->mgmtData;
    int i;
   
    for (i = 0; i < bufferSize; i++) {
        if (pageFrame[i].pageNum == page->pageNum) {
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
        if (pageFrame[i].pageNum == page->pageNum) {
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
        if (pageFrame[i].pageNum == page->pageNum) {
            SM_FileHandle fh;
            openPageFile(bm->pageFile, &fh);
            writeBlock(pageFrame[i].pageNum, &fh, pageFrame[i].data);
            pageFrame[i].dirtyBit = 0;
            writeCount++;
        }
    }
    return RC_OK;
}

extern RC pinPage(BM_BufferPool *const bm, BM_PageHandle *const page,
                  const PageNumber pageNum) {
    PageFrame *pageFrame = (PageFrame *)bm->mgmtData;
   
    if (pageFrame[0].pageNum == -1) {
        SM_FileHandle fh;
        openPageFile(bm->pageFile, &fh);
        pageFrame[0].data = (SM_PageHandle) malloc(PAGE_SIZE);
        ensureCapacity(pageNum, &fh);
        readBlock(pageNum, &fh, pageFrame[0].data);
        pageFrame[0].pageNum = pageNum;
        pageFrame[0].fixCount++;
        rearIndex = hit = 0;
        pageFrame[0].hitNum = hit;
        pageFrame[0].refNum = 0;
        page->pageNum = pageNum;
        page->data = pageFrame[0].data;
        return RC_OK;
    } else {
        int i;
        bool isBufferFull = true;
       
        for (i = 0; i < bufferSize; i++) {
            if (pageFrame[i].pageNum != -1) {
                if (pageFrame[i].pageNum == pageNum) {
                    pageFrame[i].fixCount++;
                    isBufferFull = false;
                    hit++;

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
                SM_FileHandle fh;
                openPageFile(bm->pageFile, &fh);
                pageFrame[i].data = (SM_PageHandle) malloc(PAGE_SIZE);
                readBlock(pageNum, &fh, pageFrame[i].data);
                pageFrame[i].pageNum = pageNum;
                pageFrame[i].fixCount = 1;
                pageFrame[i].refNum = 0;
                rearIndex++;
                hit++;

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
            rearIndex++;
            hit++;

            if (bm->strategy == RS_LRU)
                newPage->hitNum = hit;
            else if (bm->strategy == RS_CLOCK)
                newPage->hitNum = 1;

            page->pageNum = pageNum;
            page->data = newPage->data;

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

extern PageNumber *getFrameContents(BM_BufferPool *const bm) {
    PageNumber *frameContents = malloc(sizeof(PageNumber) * bufferSize);
    PageFrame *pageFrame = (PageFrame *) bm->mgmtData;
    int i = 0;
   
    while (i < bufferSize) {
        frameContents[i] = (pageFrame[i].pageNum != -1) ? pageFrame[i].pageNum : NO_PAGE;
        i++;
    }
    return frameContents;
}

extern bool *getDirtyFlags(BM_BufferPool *const bm) {
    bool *dirtyFlags = malloc(sizeof(bool) * bufferSize);
    PageFrame *pageFrame = (PageFrame *)bm->mgmtData;
    int i;
   
    for (i = 0; i < bufferSize; i++) {
        dirtyFlags[i] = (pageFrame[i].dirtyBit == 1) ? true : false ;
    }
    return dirtyFlags;
}

extern int getNumReadIO(BM_BufferPool *const bm) {
    return (rearIndex + 1);
}

extern int getNumWriteIO(BM_BufferPool *const bm) {
    return writeCount;
}








extern int *getFixCounts(BM_BufferPool *const bm) {
    int *fixCounts = malloc(sizeof(int) * bufferSize);
    PageFrame *pageFrame= (PageFrame *)bm->mgmtData;
    int i = 0;
   
    while (i < bufferSize) {
        fixCounts[i] = (pageFrame[i].fixCount != -1) ? pageFrame[i].fixCount : 0;
        i++;
    }
    return fixCounts;
}



