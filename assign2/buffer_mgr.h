#ifndef BUFFER_MANAGER_H
#define BUFFER_MANAGER_H

// Include return codes and methods for logging errors
#include "dberror.h"

// Include bool DT
#include "dt.h"

// Replacement Strategies
typedef enum ReplacementStrategy {
	RS_FIFO = 0,
	RS_LRU = 1,
	RS_CLOCK = 2,
	RS_LFU = 3,
	RS_LRU_K = 4
} ReplacementStrategy;

// Data Types and Structures
typedef int PageNumber;
#define NO_PAGE -1

// buffer_mgr.h
typedef struct BM_BufferPool {
    char *pageFile;                // Page file name
    int numPages;                  // Number of pages in the buffer pool
    ReplacementStrategy strategy;  // Page replacement strategy (LRU, FIFO, etc.)
    void *mgmtData;                // Management data (frames or additional metadata)
    int stratParam;                // Add stratParam here for strategy-specific data
} BM_BufferPool;



typedef struct BM_PageHandle {
	PageNumber pageNum;
	char *data;
} BM_PageHandle;

// convenience macros
#define MAKE_POOL()					\
		((BM_BufferPool *) malloc (sizeof(BM_BufferPool)))

#define MAKE_PAGE_HANDLE()				\
		((BM_PageHandle *) malloc (sizeof(BM_PageHandle)))

// Buffer Manager Interface Pool Handling
extern RC initBufferPool(BM_BufferPool *const bm, const char *const pageFileName, 
		const int numPages, ReplacementStrategy strategy,
		void *stratData);
extern RC shutdownBufferPool(BM_BufferPool *const bm);
extern RC forceFlushPool(BM_BufferPool *const bm);

// Buffer Manager Interface Access Pages
extern RC markDirty (BM_BufferPool *const bm, BM_PageHandle *const page);
extern RC unpinPage (BM_BufferPool *const bm, BM_PageHandle *const page);
extern RC forcePage (BM_BufferPool *const bm, BM_PageHandle *const page);
extern RC pinPage (BM_BufferPool *const bm, BM_PageHandle *const page, const PageNumber pageNum);

// Statistics Interface
extern PageNumber *getFrameContents (BM_BufferPool *const bm);
extern bool *getDirtyFlags (BM_BufferPool *const bm);
extern int *getFixCounts (BM_BufferPool *const bm);
extern int getNumReadIO (BM_BufferPool *const bm);
extern int getNumWriteIO (BM_BufferPool *const bm);

typedef struct Frames {
    int pageNumber;      // The page number stored in this frame
    char *memPage;       // Pointer to the data in this frame (memory page)
    bool dirty;          // Whether the page is dirty (modified)
    int fix_cnt;         // Fix count to track how many times this page is pinned
    int lruOrder;        // An example field for LRU ordering (optional)
} Frames;

#endif
