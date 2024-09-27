# BUFFER MANAGER

The buffer manager in C is a vital component of database systems that efficiently manages memory buffers for data pages. It maintains a buffer pool that temporarily stores pages retrieved from disk, reducing the need for costly I/O operations. Implementing various page replacement strategies like FIFO, LRU, LFU, and CLOCK, the buffer manager decides which pages to evict when new pages are loaded. It also tracks page usage by allowing clients to pin and unpin pages, ensuring data integrity. Additionally, the buffer manager manages dirty pages, writing modified data back to disk as needed, and provides statistics for performance monitoring and optimization.

# RUNNING THE MAKEFILE

Open code editor
Open terminal
Run “make clean” to clean the compiled files, executable files and log files if there is any:
make clean
Type "make run_test_1" to run "test_assign2_1.c" file.
Type "make run_test_2" to run "test_assign2_2.c" file.


# INCLUDED FILES:

	Makefile
	README.txt
	buffer_mgr.c
	buffer_mgr.h
	buffer_mgr_stat.c
	buffer_mgr_stat.h
	dberror.c
	dberror.h
	dt.h
	storage_mgr.c
	storage_mgr.h
	test_assign2_1.c
	test_assign2_2.c
	test_helper.h

## BUFFER POOL FUNCTIONS
---------------------------------------------------------------------------------------------------------------------------------
- initBufferPool(...) This function initializes a new buffer pool in memory. It takes the following parameters: numPages, which specifies the number of page frames that can be accommodated in the buffer; pageFileName, which indicates the name of the page file to be cached; strategy, which defines the page replacement strategy (such as FIFO, LRU, LFU, or CLOCK); and stratData, which can carry additional parameters for the chosen page replacement strategy.

- shutdownBufferPool(...) This function effectively terminates and destroys the buffer pool. It first calls forceFlushPool(...), ensuring that all modified pages (those with the dirty bit set) are written to the disk. If any pages are currently being utilized by clients, it returns the RC_PINNED_PAGES_IN_BUFFER error to indicate that resources cannot be freed.

- forceFlushPool(...) This function is responsible for writing all dirty pages (pages marked with a dirty bit of 1) back to the disk. It scans through each page frame in the buffer pool, checking if the dirty bit is set to 1 and if the fix count is 0 (indicating that no user is currently using that page). If both conditions are met, the page frame's contents are written to the disk.

## PAGE MANAGEMENT FUNCTIONS
---------------------------------------------------------------------------------------------------------------------------------
- pinPage(...) This function pins a specified page (identified by pageNum) by reading it from the page file on disk and storing it in the buffer pool. Before pinning, it checks whether there is available space in the buffer pool. If space is unavailable, it employs a page replacement strategy to replace an existing page. The chosen page is examined to determine if it is dirty; if so, its contents are written back to disk before adding the new page.

- unpinPage(...) This function unpins a specified page, identified by its page number. It locates the page within the buffer pool and decrements its fix count, indicating that the client has finished using it.

- makeDirty(...) This function sets the dirty bit of a specific page frame to 1. It searches for the page frame corresponding to the provided page number and, upon locating it, marks the dirty bit accordingly.

- forcePage(...) This function writes the contents of a specified page frame back to the page file on disk. It identifies the page by checking the page numbers in the buffer pool. When found, it uses the Storage Manager functions to perform the write operation, and after successfully writing, it resets the dirty bit to 0.

## STATISTICS FUNCTIONS
---------------------------------------------------------------------------------------------------------------------------------
- getFrameContents(...) This function returns an array of page numbers corresponding to the pages currently stored in the buffer pool. The size of the array matches the buffer size (numPages), and each element holds the page number for the respective page frame.

- getDirtyFlags(...) This function returns an array of boolean values indicating the dirty state of each page frame in the buffer pool. The array size is equal to the number of pages, where each element reflects whether the respective page is dirty (TRUE) or clean (FALSE).

- getFixCounts(...) This function provides an array of integers, where each element represents the fix count for the pages currently in the buffer pool. The size of the array matches the buffer size, indicating how many clients are using each page.

- getNumReadIO(...) This function returns the total count of I/O read operations conducted by the buffer pool, reflecting the number of pages that have been read from the disk. This count is maintained using the rearIndex variable.

- getNumWriteIO(...) This function returns the total count of I/O write operations performed by the buffer pool, indicating how many pages have been written to the disk. The writeCount variable tracks this information, which is initialized to 0 when the buffer pool is created and incremented with each write operation.

## PAGE REPLACEMENT ALGORITHM FUNCTIONS
---------------------------------------------------------------------------------------------------------------------------------
The functions implementing page replacement strategies—FIFO, LRU, LFU, and CLOCK—are utilized when a new page needs to be pinned, and the buffer pool is full. These strategies help decide which page should be replaced.

- FIFO(...) The First In First Out (FIFO) strategy operates like a queue, replacing the oldest page that was added to the buffer pool first. When a page needs to be evicted, the oldest page's contents are written to disk, and the new page is then added to that slot.

- LFU(...) The Least Frequently Used (LFU) strategy removes the page that has been accessed the least number of times. Each page frame has a reference count (refNum) that tracks its access frequency. During LFU replacement, the page frame with the lowest refNum is evicted, its contents are written to disk, and the new page is added.

- LRU(...) The Least Recently Used (LRU) strategy evicts the page that has not been accessed for the longest time. Each page frame maintains a hit count (hitNum) indicating how recently it was accessed. The page with the lowest hitNum is selected for eviction, its contents are written back to disk, and the new page is inserted in its place.

- CLOCK(...) The CLOCK algorithm keeps track of the last added page and uses a pointer (clockPointer) to determine which page frame to replace. When a replacement is needed, it checks the hit count of the page at the clockPointer. If the hit count is not 1, that page is evicted; if it is 1, the hit count is reset to 0, and the pointer advances to the next page. This process continues until a suitable page is found for replacement, preventing infinite loops by resetting the hit count.