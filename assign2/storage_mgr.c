#include "storage_mgr.h" //Adding header file for the interface
#include "dberror.h" //Error code header file
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


//Initially, we have to initialize the storage manager
extern void initStorageManager(void){
    printf("The storage manager is initialized!!");
}

/*********************** FILE OPERATIONS *********************/

//WORK DONE BY RUDRA PATEL (A20594446)

//Creating a page file
extern RC createPageFile(char *fileName) {
    FILE *fileopen = fopen(fileName, "w"); // Opening the file to write
    if (fileopen == NULL) {
        printf("Error: File could not be created.\n");
        fflush(stdout);
        return RC_FILE_NOT_FOUND;   // If not opened, return ERROR
    }
    
    printf("File created successfully.\n");
    fflush(stdout); // Force message to be printed immediately

    // Create an empty page and allocate some memory for one page
    char *newpage = (char *)calloc(PAGE_SIZE, sizeof(char));
    if (!newpage) {
        fclose(fileopen);
        printf("Error: Memory allocation failed.\n");
        fflush(stdout);
        return RC_WRITE_FAILED;
    }

    size_t write_code = fwrite(newpage, sizeof(char), PAGE_SIZE, fileopen);

    // Check if the write operation succeeded
    if (write_code != PAGE_SIZE) {
        free(newpage);
        fclose(fileopen);
        printf("Error: Write operation failed.\n");
        fflush(stdout);
        return RC_WRITE_FAILED;
    }

    printf("Write operation successful.\n");
    fflush(stdout); // Ensure the success message is printed

    // Free all allocated memory after writing
    free(newpage);
    fclose(fileopen);

    printf("File closed successfully.\n");
    fflush(stdout);

    return RC_OK;
}


//WORK DONE BY SABARISH RAJA RAMESH RAJA (A20576363)
//OPEN AN EXISTING PAGE FILE
extern RC openPageFile (char *fileName, SM_FileHandle *fHandle){
    FILE *file = fopen(fileName, "r+");  //Opening file for reading and writing
    if(file == NULL){
        return RC_FILE_NOT_FOUND;
    }
    //Set file handle field to store information about file
    //in the SM_FileHandle
    fHandle-> fileName = fileName; //Storing file name
    fHandle-> curPagePos = 0; //Setting the current page to 1st page

    //We can calculate the total number of pages in the file by getting file size
    fseek(file, 0L, SEEK_END); //Moving to end of the file
    long fileSize = ftell(file); //Get size of the file
    fHandle -> totalNumPages = (int) (fileSize/PAGE_SIZE);

    fHandle->mgmtInfo = file;
    return RC_OK;

}

extern RC closePageFile (SM_FileHandle *fHandle){
    FILE *closefile = (FILE *)fHandle ->mgmtInfo; // getting the file pointer from mgmtInfo
    if(fclose(closefile) == 0){ // Close the file successfully
        return RC_OK;
    }
    else{
        return RC_FILE_NOT_FOUND; // Return an appropriate error code
    }
}

//Destroying a page file
extern RC destroyPageFile (char *fileName){
    if(remove(fileName) == 0){ // If the file is removed successfully
        return RC_OK;
    } else {
        return RC_FILE_NOT_FOUND; // Return an error if the file could not be removed
    }
}

/*********************** READ OPERATIONS *********************/
extern RC readBlock (int pageNum, SM_FileHandle *fHandle, SM_PageHandle memPage){
    //Initially, checking if the page number is valid:
    if(pageNum<0 || pageNum > fHandle->totalNumPages ){
        //If page doesn't exist
        return RC_READ_NON_EXISTING_PAGE;
    }
    //Creating a pointer for file using the additional information
    FILE *readblockfile = (FILE *)fHandle->mgmtInfo;
    //Steps => Get the File from fHandle -> Seek the page block using offset
    long offset = pageNum * PAGE_SIZE;
    //Move the pointer to current page
    int seek_pointer = fseek(readblockfile, offset, SEEK_SET);
    if(seek_pointer!=0){
        return RC_READ_NON_EXISTING_PAGE;
    }
    //Goal: Read a specific page from a file into memory
    size_t read = fread(memPage, sizeof(char), PAGE_SIZE, readblockfile);
    //SYNTAX: FREAD(VOID *PTR(destination where data is stored), 
    // SIZE_T SIZE (size of element to read), 
    // SIZE_T COUNT(number of elements to read), 
    // FILE *STREAM (file pointer from which data will be read))
    if(read<PAGE_SIZE){
        return RC_READ_NON_EXISTING_PAGE;
    }
    // Updatr the current page position to this page
    fHandle->curPagePos = pageNum;
    return RC_OK;

}

/*getBlockPos's responsibility to get the page's position from a file*/
extern int getBlockPos (SM_FileHandle *fHandle){
    //Constraints: Check if the files are present in the fHandle data structure and
    // Check if the current page is in a valid range i.e. between 0 and the total number of pages.
    if(fHandle == NULL || fHandle->mgmtInfo == NULL){
        return RC_FILE_HANDLE_NOT_INIT;
    }
    if(fHandle->curPagePos <1 || fHandle->curPagePos >= fHandle->totalNumPages){
        return RC_READ_NON_EXISTING_PAGE;
    }
    return fHandle->curPagePos;
}

//Reading the first Block of the file
extern RC readFirstBlock (SM_FileHandle *fHandle, SM_PageHandle memPage){
    //Idea: Call the readBlock() function by passing the first page's index i.e. 0
    return readBlock(0, fHandle, memPage);
}

//Reading the Last Block of the File
extern RC readLastBlock (SM_FileHandle *fHandle, SM_PageHandle memPage){
    int lastPage = fHandle->totalNumPages-1;
    return readBlock(lastPage, fHandle, memPage);
}

//Reading the previous block from the current block 
extern RC readPreviousBlock (SM_FileHandle *fHandle, SM_PageHandle memPage){
    int previousPage = fHandle->curPagePos - 1;
    //Check if the page number is not below 0
    if(previousPage <= 0){
        return RC_READ_NON_EXISTING_PAGE;
    }
    return readBlock(previousPage, fHandle, memPage);
}

//Reading the current block from file
extern RC readCurrentBlock (SM_FileHandle *fHandle, SM_PageHandle memPage){
    int currentPage = fHandle->curPagePos;
    return readBlock(currentPage, fHandle, memPage);
}

//Reading the Next Block from file
extern RC readNextBlock (SM_FileHandle *fHandle, SM_PageHandle memPage){
    int nextPage = fHandle->curPagePos + 1;
    //The next block fo file should not be more than total number of pages
    if(nextPage>=fHandle->totalNumPages){
        return RC_READ_NON_EXISTING_PAGE;
    }
    return readBlock(nextPage, fHandle, memPage);
}

/*********************** WRITE OPERATIONS *********************/

//WORK DONE BY RUDRA PATEL (A20594446)
extern RC writeBlock (int pageNum, SM_FileHandle *fHandle, SM_PageHandle memPage) {
    // Check if the page number is valid
    if (pageNum < 0 || pageNum >= fHandle->totalNumPages) {
        return RC_WRITE_FAILED;
    }

    // Create a file pointer using the information in fHandle
    FILE *file = (FILE *) fHandle->mgmtInfo;

    // Move the pointer to the correct page location
    long offset = pageNum * PAGE_SIZE;
    if (fseek(file, offset, SEEK_SET) != 0) {
        return RC_WRITE_FAILED;
    }

    // Write the block of memory to the file
    size_t write_size = fwrite(memPage, sizeof(char), PAGE_SIZE, file);
    if (write_size < PAGE_SIZE) {
        return RC_WRITE_FAILED;
    }

    // Update the current page position in the file handle
    fHandle->curPagePos = pageNum;

    return RC_OK;
}

extern RC writeCurrentBlock(SM_FileHandle *fHandle, SM_PageHandle memPage) {
    // Ensure that the current page position is valid
    if (fHandle->curPagePos < 0 || fHandle->curPagePos >= fHandle->totalNumPages) {
        return RC_WRITE_FAILED;
    }

    // Use writeBlock to write to the current page
    return writeBlock(fHandle->curPagePos, fHandle, memPage);
}

extern RC appendEmptyBlock(SM_FileHandle *fHandle) {
    // Allocate memory for an empty page and set it to 0
    SM_PageHandle emptyPage = (SM_PageHandle) calloc(PAGE_SIZE, sizeof(char));
    if (emptyPage == NULL) {
        return RC_WRITE_FAILED;  // Memory allocation failed
    }

    // Retrieve the file pointer from the file handle (stored in mgmtInfo)
    FILE *file = (FILE *) fHandle->mgmtInfo;

    // Move the file pointer to the end of the file
    if (fseek(file, 0L, SEEK_END) != 0) {
        free(emptyPage);
        return RC_FILE_NOT_FOUND;  // File seek error
    }

    // Write the empty page (filled with zero bytes) to the file
    size_t write_code = fwrite(emptyPage, sizeof(char), PAGE_SIZE, file);
    if (write_code != PAGE_SIZE) {
        free(emptyPage);
        return RC_WRITE_FAILED;  // Error in writing to the file
    }

    // Update the file handle's total number of pages
    fHandle->totalNumPages++;

    // Free the allocated memory for the empty page
    free(emptyPage);

    // Return success
    return RC_OK;
}


//WORK DONE BY RUDRA PATEL (A20594446)
RC ensureCapacity(int requiredPages, SM_FileHandle *fileHandle) {
    FILE *pageFile = (FILE *)fileHandle->mgmtInfo;  // Cast management info to FILE pointer

    // If the file already has the required number of pages, no action is needed
    if (fileHandle->totalNumPages >= requiredPages) {
        return RC_OK;
    }

    // Calculate how many pages need to be added
    int additionalPages = requiredPages - fileHandle->totalNumPages;

    // Create a buffer for an empty page, filled with zeros
    SM_PageHandle newPageBuffer = (SM_PageHandle)calloc(PAGE_SIZE, sizeof(char));

    // If memory allocation fails, return an error code
    if (newPageBuffer == NULL) {
        return RC_WRITE_FAILED;
    }

    // Move the file pointer to the end of the file
    if (fseek(pageFile, 0, SEEK_END) != 0) {
        free(newPageBuffer); // Free buffer if file seek fails
        return RC_WRITE_FAILED;
    }

    // Add the necessary number of blank pages to the file
    for (int i = 0; i < additionalPages; i++) {
        if (fwrite(newPageBuffer, sizeof(char), PAGE_SIZE, pageFile) != PAGE_SIZE) {
            free(newPageBuffer); // Free the buffer if writing fails
            return RC_WRITE_FAILED;
        }
    }

    // Update the file handle's total number of pages
    fileHandle->totalNumPages = requiredPages;

    // Free the allocated memory
    free(newPageBuffer);

    return RC_OK; // Return success
}
