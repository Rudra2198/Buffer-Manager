#include<stdio.h>
#include<stdlib.h>
#include<sys/stat.h>
#include<sys/types.h>
#include<unistd.h>
#include<string.h>
#include<math.h>

#include "storage_mgr.h"

FILE *pageFile;

extern void initStorageManager (void) {
	// Initialising file pointer i.e. storage manager.
	pageFile = NULL;
}

extern RC createPageFile (char *fileName) {
	// Opening file stream in read & write mode. 'w+' mode creates an empty file for both reading and writing.
	pageFile = fopen(fileName, "w+");

	// Checking if file was successfully opened.
	if(pageFile == NULL) {
		return RC_FILE_NOT_FOUND;
	} else {
		// Creating an empty page in memory.
		SM_PageHandle emptyPage = (SM_PageHandle)calloc(PAGE_SIZE, sizeof(char));
		
		// Writing empty page to file.
		if(fwrite(emptyPage, sizeof(char), PAGE_SIZE,pageFile) < PAGE_SIZE)
			printf("write failed \n");
		else
			printf("write succeeded \n");
		
		// Closing file stream so that all the buffers are flushed. 
		fclose(pageFile);
		
		// De-allocating the memory previously allocated to 'emptyPage'.
		// This is optional but always better to do for proper memory management.
		free(emptyPage);
		
		return RC_OK;
	}
}

extern RC openPageFile (char *fileName, SM_FileHandle *fHandle) {
	// Opening file stream in read mode. 'r' mode creates an empty file for reading only.
	pageFile = fopen(fileName, "r");

	// Checking if file was successfully opened.
	if(pageFile == NULL) {
		return RC_FILE_NOT_FOUND;
	} else { 
		// Updating file handle's filename and set the current position to the start of the page.
		fHandle->fileName = fileName;
		fHandle->curPagePos = 0;

		/* In order to calculate the total size, we perform following steps -
		   1. Move the position of the file stream to the end of file
		   2. Check the file end position
		   3. Move the position of the file stream to the beginning of file  
		
		fseek(pageFile, 0L, SEEK_END);
		int totalSize = ftell(pageFile);
		fseek(pageFile, 0L, SEEK_SET);
		fHandle->totalNumPages = totalSize/ PAGE_SIZE;  */
		
		/* Using fstat() to get the file total size.
		   fstat() is a system call that is used to determine information about a file based on its file descriptor.
		   'st_size' member variable of the 'stat' structure gives the total size of the file in bytes.
		*/

		struct stat fileInfo;
		if(fstat(fileno(pageFile), &fileInfo) < 0)    
			return RC_ERROR;
		fHandle->totalNumPages = fileInfo.st_size/ PAGE_SIZE;

		// Closing file stream so that all the buffers are flushed. 
		fclose(pageFile);
		return RC_OK;
	}
}

extern RC closePageFile (SM_FileHandle *fHandle) {
	// Checking if file pointer or the storage manager is intialised. If initialised, then close.
	if(pageFile != NULL)
		pageFile = NULL;	
	return RC_OK; 
}


extern RC destroyPageFile (char *fileName) {
	// Opening file stream in read mode. 'r' mode creates an empty file for reading only.	
	pageFile = fopen(fileName, "r");
	
	if(pageFile == NULL)
		return RC_FILE_NOT_FOUND; 
	
	// Deleting the given filename so that it is no longer accessible.	
	remove(fileName);
	return RC_OK;
}

extern RC readBlock (int pageNum, SM_FileHandle *fHandle, SM_PageHandle memPage) {
	// Checking if the pageNumber parameter is less than Total number of pages and less than 0, then return respective error code
	if (pageNum > fHandle->totalNumPages || pageNum < 0)
        	return RC_READ_NON_EXISTING_PAGE;

	// Opening file stream in read mode. 'r' mode opens file for reading only.	
	pageFile = fopen(fHandle->fileName, "r");

	// Checking if file was successfully opened.
	if(pageFile == NULL)
		return RC_FILE_NOT_FOUND;
	
	// Setting the cursor(pointer) position of the file stream. Position is calculated by Page Number x Page Size
	// And the seek is success if fseek() return 0
	int isSeekSuccess = fseek(pageFile, (pageNum * PAGE_SIZE), SEEK_SET);
	if(isSeekSuccess == 0) {
		// We're reading the content and storing it in the location pointed out by memPage.
		if(fread(memPage, sizeof(char), PAGE_SIZE, pageFile) < PAGE_SIZE)
			return RC_ERROR;
	} else {
		return RC_READ_NON_EXISTING_PAGE; 
	}
    	
	// Setting the current page position to the cursor(pointer) position of the file stream
	fHandle->curPagePos = ftell(pageFile); 
	
	// Closing file stream so that all the buffers are flushed.     	
	fclose(pageFile);
	
    	return RC_OK;
}

extern int getBlockPos (SM_FileHandle *fHandle) {
	// Returning the current page position retrieved from the file handle	
	return fHandle->curPagePos;
}

extern RC readFirstBlock (SM_FileHandle *fHandle, SM_PageHandle memPage) {
	// Opening file stream in read mode. 'r' mode opens the file for reading only.	
	pageFile = fopen(fHandle->fileName, "r");
	
	// Checking if file was successfully opened.
	if(pageFile == NULL)
		return RC_FILE_NOT_FOUND;

	int i;
	for(i = 0; i < PAGE_SIZE; i++) {
		// Reading a single character from the file
		char c = fgetc(pageFile);
	
		// Checking if we have reached the end of file
		if(feof(pageFile))
			break;
		else
			memPage[i] = c;
	}

	// Setting the current page position to the cursor(pointer) position of the file stream
	fHandle->curPagePos = ftell(pageFile); 

	// Closing file stream so that all the buffers are flushed.
	fclose(pageFile);
	return RC_OK;
}

extern RC readPreviousBlock (SM_FileHandle *fHandle, SM_PageHandle memPage) {
	//printf("CURRENT PAGE POSITION = %d \n", fHandle->curPagePos);
	//printf("TOTAL PAGES = %d \n", fHandle->totalNumPages)

	// Checking if we are on the first block because there's no other previous block to read
	if(fHandle->curPagePos <= PAGE_SIZE) {
		printf("\n First block: Previous block not present.");
		return RC_READ_NON_EXISTING_PAGE;	
	} 
    else {
		
        // Calculating current page number by dividing page size by current page position	
		
        
        int currentPageNumber = fHandle->curPagePos / PAGE_SIZE;
		int startPosition = (PAGE_SIZE * (currentPageNumber - 2));

		// Opening file stream in read mode. 'r' mode opens the file for reading only.	
		pageFile = fopen(fHandle->fileName, "r");
		
		// Checking if file was successfully opened.
		if(pageFile == NULL)
			return RC_FILE_NOT_FOUND;

		// Initializing file pointer position.
		fseek(pageFile, startPosition, SEEK_SET);
		
		int i;
		// Reading block character by character and storing it in memPage
		for(i = 0; i < PAGE_SIZE; i++) {
			memPage[i] = fgetc(pageFile);
		}

		// Setting the current page position to the cursor(pointer) position of the file stream
		fHandle->curPagePos = ftell(pageFile); 

		// Closing file stream so that all the buffers are flushed.
		fclose(pageFile);
		return RC_OK;
	}
}

extern RC readCurrentBlock (SM_FileHandle *fHandle, SM_PageHandle memPage) {
	// Calculating current page number by dividing page size by current page position	
	int currentPageNumber = fHandle->curPagePos / PAGE_SIZE;
	
    int startPosition = (PAGE_SIZE * (currentPageNumber - 2));
	
	// Opening file stream in read mode. 'r' mode opens the file for reading only.	
	pageFile = fopen(fHandle->fileName, "r");

	// Checking if file was successfully opened.
	if(pageFile == NULL)

		return RC_FILE_NOT_FOUND;

	// Initializing file pointer position.
	fseek(pageFile, startPosition, SEEK_SET);
	
	int i;
	// Reading block character by character and storing it in memPage.
	// Also checking if we have reahed end of file.
	for(i = 0; i < PAGE_SIZE; i++) {
		char c = fgetc(pageFile);		
		if(feof(pageFile))
			break;
		memPage[i] = c;
	}
	
	// Setting the current page position to the cursor(pointer) position of the file stream
	fHandle->curPagePos = ftell(pageFile); 

	// Closing file stream so that all the buffers are flushed.
	fclose(pageFile);
	return RC_OK;		
}

extern RC readNextBlock(SM_FileHandle *fHandle, SM_PageHandle memPage) {
    // Check if the current position is at the last block, which means no next block exists
    if (fHandle->curPagePos == PAGE_SIZE) {
        printf("\n Last block: Next block not present.");
        return RC_READ_NON_EXISTING_PAGE;
    } else {
        // Calculate the current page number based on the current position
        int pageNumber = fHandle->curPagePos / PAGE_SIZE;
        int readPosition = (PAGE_SIZE * (pageNumber - 2));

        // Open the file in read mode
        FILE *file = fopen(fHandle->fileName, "r");

        // Check if the file opened successfully
        if (file == NULL)
            return RC_FILE_NOT_FOUND;

        // Move the file pointer to the start position of the next block
        fseek(file, readPosition, SEEK_SET);

        // Read the block into the memory page buffer
        for (int i = 0; i < PAGE_SIZE; i++) {
            char ch = fgetc(file);
            if (feof(file))
                break;
            memPage[i] = ch;
        }

        // Update the file handle's current page position
        fHandle->curPagePos = ftell(file);

        // Close the file after reading
        fclose(file);
        return RC_OK;
    }
}

extern RC readLastBlock(SM_FileHandle *fHandle, SM_PageHandle memPage) {
    // Open the file in read mode to read the last block
    FILE *file = fopen(fHandle->fileName, "r");

    // Check if the file opened successfully
    if (file == NULL)
        return RC_FILE_NOT_FOUND;

    // Calculate the start position of the last page
    int readPosition = (fHandle->totalNumPages - 1) * PAGE_SIZE;

    // Set the file pointer to the start of the last block
    fseek(file, readPosition, SEEK_SET);

    // Read characters from the file into the memory page buffer
    for (int i = 0; i < PAGE_SIZE; i++) {
        char ch = fgetc(file);
        if (feof(file))
            break;
        memPage[i] = ch;
    }

    // Update the current position of the file handle
    fHandle->curPagePos = ftell(file);

    // Close the file after reading
    fclose(file);
    return RC_OK;
}

extern RC writeBlock(int pageNum, SM_FileHandle *fHandle, SM_PageHandle memPage) {
    // Check if the page number is within valid bounds
    if (pageNum > fHandle->totalNumPages || pageNum < 0)
        return RC_WRITE_FAILED;

    // Open the file in read-write mode
    FILE *file = fopen(fHandle->fileName, "r+");

    // Check if the file opened successfully
    if (file == NULL)
        return RC_FILE_NOT_FOUND;

    // Calculate the starting position to write to the block
    int writePosition = pageNum * PAGE_SIZE;

    if (pageNum == 0) {
        // Write data directly to the specified page
        fseek(file, writePosition, SEEK_SET);
        for (int i = 0; i < PAGE_SIZE; i++) {
            // Append an empty block if the end of file is reached during writing
            if (feof(file))
                appendEmptyBlock(fHandle);
            fputc(memPage[i], file);
        }

        // Update the current position of the file handle
        fHandle->curPagePos = ftell(file);

        // Close the file after writing
        fclose(file);
    } else {
        // Handle writing to the first page separately
        fHandle->curPagePos = writePosition;
        fclose(file);
        writeCurrentBlock(fHandle, memPage);
    }
    return RC_OK;
}



extern RC writeCurrentBlock(SM_FileHandle *fHandle, SM_PageHandle memPage) {
    // Open the file in read-write mode to write to the current block
    FILE *file = fopen(fHandle->fileName, "r+");

    // Check if the file opened successfully
    if (file == NULL)
        return RC_FILE_NOT_FOUND;

    // Add an empty block if needed before writing
    appendEmptyBlock(fHandle);

    // Move the file pointer to the current position
    fseek(file, fHandle->curPagePos, SEEK_SET);

    // Write the content of the memory page to the file
    fwrite(memPage, sizeof(char), strlen(memPage), file);

    // Update the file handle's current page position
    fHandle->curPagePos = ftell(file);

    // Close the file
    fclose(file);
    return RC_OK;
}

extern RC appendEmptyBlock(SM_FileHandle *fHandle) {
    // Allocate a blank page of size PAGE_SIZE bytes
    SM_PageHandle emptyPage = (SM_PageHandle)calloc(PAGE_SIZE, sizeof(char));

    // Move the file pointer to the end of the file
    FILE *file = fopen(fHandle->fileName, "a+");

    // Write the empty page to the file to create space
    if (fwrite(emptyPage, sizeof(char), PAGE_SIZE, file) != PAGE_SIZE) {
        free(emptyPage);
        fclose(file);
        return RC_WRITE_FAILED;
    }

    // Free the allocated memory for the empty page buffer
    free(emptyPage);

    // Increment the total number of pages in the file handle
    fHandle->totalNumPages++;

    // Close the file after appending
    fclose(file);
    return RC_OK;
}

extern RC ensureCapacity(int numberOfPages, SM_FileHandle *fHandle) {
    // Open the file in append mode to ensure capacity
    FILE *file = fopen(fHandle->fileName, "a");

    // Check if the file opened successfully
    if (file == NULL)
        return RC_FILE_NOT_FOUND;

    // Add empty blocks until the total number of pages matches the required capacity
    while (numberOfPages > fHandle->totalNumPages)
        appendEmptyBlock(fHandle);

    // Close the file stream after ensuring capacity
    fclose(file);
    return RC_OK;
}
