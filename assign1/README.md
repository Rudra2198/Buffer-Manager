Storage Manager
---------------------------------------

Team members
---------------------------------------
1) Sabarish Raja Ramesh Raja (A20576363)

2) Rudra Hirenkumar Patel (A20594446)

3) Enoch Ashong (A20555274)

// WORK DONE by ENOCH ASHONG (A20555274)

Aim
----------------------------------------------------------------------------------------
The objective of this assignment is to implementa storage manager that is capable of reading blocks from a file on disk into memory and writing blocks from memory to a file on disk. The file has fixed size of blocks called pages.

FILE(has Pages/Blocks) _READ_> Memory

Memory _WRITE_> File (Present on Disk)

The various file operations we have in the assignment are:<br/>
* createPageFile()
* openPageFile()
* closePageFile()
* destroyPageFile()

The read operation functions are:
1. readBlock()
2. getBlockPos()
3. readFirstBlock()
4. readPreviousBlock()
5. readCurrentBlock()
6. readNextBlock()
7. readLastBlock()

The write operations are:
1. writeBlock()
2. writeCurrentBlock()
3. appendEmptyBlock()
4. ensureCapacity()

The files that are provided for the assignment are:
I) dberror.h -> Header file that'll have all error message constants
ii) dberror.c -> function definition for error messages.
iii) storage_mgr.h -> Header file that'll have all the declared functions that are defined in .c
iv) test_assign1_1.c -> Test cases for the code 

Files to be created:
I) storage_mgr.c -> Define all the operations for read, write, file operations, etc.

There are 2 main important data structures in this assignment:
1. File Handle (SM_FileHandle) and,
2. Page Handle (SM_PageHandle)

File Handle represents an open page file that has the following details about the file
1) File name
2) Total number of pages
3) Current Page Position
4) mgmtInfo -> It stores the additional information about file

Page Handle is a pointer to an area in memory where the content of a page is stored.

1. createPageFile()
----------------------------------------------------------------------------------------
Purpose: Creates a new file and initializes it with an empty page of data.

- Opens the file in write mode.
- Returns RC_FILE_NOT_FOUND if the file cannot be opened.
- Allocates memory for one page using calloc
  (ensuring the memory  is initialized to zero).
- Writes the empty page to the file using fwrite.
- Closes the file on success and returns RC_OK.
- Returns RC_WRITE_FAILED on failure.


2. openPageFile()
----------------------------------------------------------------------------------------
Purpose: Opens an existing file and initializes the SM_FileHandle structure.

- Opens the file in read/write mode.
- Returns RC_FILE_NOT_FOUND if the file cannot be opened.
- Sets the fileName and initializes curPagePos to 0 
  (indicating the first page).
- Calculates the total number of pages by determining 
  the file size.
- Stores the file pointer in the mgmtInfo field for 
  future operations.
- Returns RC_OK on success.

3. closePageFile()
----------------------------------------------------------------------------------------
Purpose: Closes an open file.

- Closes the file using the file pointer stored in mgmtInfo.
- Returns RC_OK on success.
- If the file fails to close, the file pointer is set to NULL.


4. destroyPageFile()
----------------------------------------------------------------------------------------
Purpose: Deletes the specified file.

- Deletes the file using the remove function.
- Returns RC_OK if the file is successfully deleted.
- Assumes failure if the file is not deleted.


5. writeBlock
----------------------------------------------------------------------------------------
Purpose: Writes data to a specific page in the file.

- Validates the page number.
- Returns RC_WRITE_FAILED if the page number is invalid.
- Calculates the correct file offset using fseek.
- Writes data from memPage to the specified page in the file 
  using fwrite.
- Updates curPagePos to reflect the current page.
- Returns RC_OK on success.

****************************************************** EXPLANATION OF READ OPERATIONS***********************************************


6. readBlock()

readBlock() function's role in the storage manager is to read a block from a file into memory.
The arguments used in the function are pageNum, fHandle and memPage
The series of steps to follow inside the function are:
* Check if the page number passed as argument is valid and return RC_READ_NON_EXISTING_PAGE if the page number is below 0 or more than the total pages.
* Create a pointer to the file using mgmtInfo which stores the additional information.
* Use fseek() to find / seek the page block using offset value(pageNum * PAGE_SIZE)
* Once the file is found, read the file using fread() and add the parameters like pointer to where data is store, the size of element to read, number of elements to read, and the file pointer from which data will be read.
* Then update the current page position (currPagePos) to the pageNum currently in.

7. getBlockPos()

getBlockPos() function's role is to get the page position from the file. It take fHandle as an argument.
The series of steps to follow are:
* If the pointer fHandle is Null or if the mgmtInfo from  fHandle is NULL, then return RC_FILE_HANDLE_NOT_INIT
* If the currentPagePos is less than 1 or if it is less than the totalNumPages, then return RC_READ_NON_EXISTING_PAGE
* At the end, return the currentPagePos which is the Block position in file.

8. readFirstBlock():

Purpose: Reads the first block of the file.

- Reads the first block by calling readBlock() with 
  the page index set to 0.
- Function call: readBlock(0, fHandle, memPage).

9. readLastBlock():

Purpose: Reads the last block of the file.

- Reads the last block by calling readBlock() with 
  the index of the last page (totalNumPages - 1).
- Function call: readBlock(fHandle->totalNumPages - 1, fHandle, memPage).

10. readPreviousBlock():

Purpose: Reads the block prior to the current block.

- Decrements curPagePos by 1 to read the previous block.
- If the previous page number is less than 0, it returns 
  an error for a non-existing page.
- Function call: readBlock(fHandle->curPagePos - 1, fHandle, memPage).

11. readCurrentBlock():

Purpose: Reads the current block of the file.

- Calls readBlock() with curPagePos as the page number.
- Function call: readBlock(fHandle->curPagePos, fHandle, memPage).

12. readNextBlock():

Purpose: Reads the block after the current block.

- Increments curPagePos by 1 to read the next block.
- If the next page exceeds the total number of pages, 
  it returns an error for a non-existing page.
- Function call: readBlock(fHandle->curPagePos + 1, fHandle, memPage).

