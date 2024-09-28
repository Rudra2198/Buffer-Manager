#include<stdio.h>
#include<stdlib.h>
#include<sys/stat.h>
#include<sys/types.h>
#include<unistd.h>
#include<string.h>
#include<math.h>

#include "storage_mgr.h"

FILE *pageFile;

extern void initStorageManager(void) {
    // Initialising file pointer, i.e., storage manager.
    pageFile = NULL; // Initial pageFile to NULL
}

extern RC createPageFile(char *fileName) {
    // Opening file stream in read & write mode. 'w+' mode creates an empty file for both reading and writing.
    pageFile = fopen(fileName, "w+");

    // Checking if file was successfully opened.
    if (pageFile == NULL) {
        return RC_FILE_NOT_FOUND; // File not found error
    }
    else {
        // Creating an empty page in memory.
        SM_PageHandle emptyPage = (SM_PageHandle)calloc(PAGE_SIZE, sizeof(char));

        // Writing empty page to file.
        
		
		
		if (fwrite(emptyPage, sizeof(char), PAGE_SIZE, pageFile) < PAGE_SIZE)
            printf("write failed \n");
        else
            printf("write succeeded \n");

        // Closing file stream so that all the buffers are flushed. 
        fclose(pageFile);

        // De-allocating memory previously allocated to 'emptyPage'.
        // This is optional but better for memory management.
        free(emptyPage);

        return RC_OK; // Success
    }
}

extern RC openPageFile(char *fileName, SM_FileHandle *fHandle) {
    // Opening file stream in read mode. 'r' mode opens for reading only.
    pageFile = fopen(fileName, "r");

    // Checking if file was successfully opened.
    if (pageFile == NULL) {
        return RC_FILE_NOT_FOUND; // File not found
    }
    else {
        // Updating file handle's filename and setting the current position to the start of the page.
        fHandle->fileName = fileName;
        
		fHandle->curPagePos = 0;

        // Using fstat() to get the file total size.
        struct stat fileInfo;
        if (fstat(fileno(pageFile), &fileInfo) < 0)
            return RC_ERROR; // Error occurred

        fHandle->totalNumPages = fileInfo.st_size / PAGE_SIZE; // Total number of pages

        // Closing file stream.
        fclose(pageFile);
        return RC_OK; // Success
    }
}

extern RC closePageFile(SM_FileHandle *fHandle) {
    // Checking if file pointer or the storage manager is initialised. If initialised, then close.
    if (pageFile != NULL)
        pageFile = NULL; // Nullifying pageFile
    return RC_OK; 
}

extern RC destroyPageFile(char *fileName) {
    // Opening file stream in read mode. 
    pageFile = fopen(fileName, "r");

    if (pageFile == NULL)
        return RC_FILE_NOT_FOUND; // File not found

    // Deleting the given filename so it is no longer accessible.
    remove(fileName); 
    return RC_OK; // Success
}

extern RC readBlock(int pageNum, SM_FileHandle *fHandle, SM_PageHandle memPage) {
    // Checking if the pageNumber parameter is less than Total number of pages and less than 0.
    
	
	
	if (pageNum >= fHandle->totalNumPages || pageNum < 0)
        return RC_READ_NON_EXISTING_PAGE; // Non-existing page error

    // Opening file stream in read mode.
    pageFile = fopen(fHandle->fileName, "r");

    // Checking if file was successfully opened.
    if (pageFile == NULL)
        return RC_FILE_NOT_FOUND; // File not found error

    // Setting cursor position of the file stream.
    int isSeekSuccess = fseek(pageFile, (pageNum * PAGE_SIZE), SEEK_SET);
    if (isSeekSuccess == 0) {
        // Reading content into memPage.
        if (fread(memPage, sizeof(char), PAGE_SIZE, pageFile) < PAGE_SIZE)
            return RC_ERROR; // Read error
    }
    else {
        return RC_READ_NON_EXISTING_PAGE; // Non-existing page error
    }

    // Updating current page position.
    fHandle->curPagePos = ftell(pageFile); 
    fclose(pageFile); // Closing the file stream
    return RC_OK; // Success
}

extern int getBlockPos(SM_FileHandle *fHandle) {
    // Returning the current page position from the file handle
    return fHandle->curPagePos;
}

extern RC readFirstBlock(SM_FileHandle *fHandle, SM_PageHandle memPage) {
    // Opening file stream in read mode.
    pageFile = fopen(fHandle->fileName, "r");

    // Checking if file was successfully opened.
    if (pageFile == NULL)
        return RC_FILE_NOT_FOUND; // File not found error

    // Reading the first block into memPage.
    for (int i = 0; i < PAGE_SIZE; i++) {
        char c = fgetc(pageFile); // Reading a single character

        if (feof(pageFile))
            break; // End of file reached
        else
            memPage[i] = c; // Storing character
    }

    // Setting the current page position to the cursor.
    fHandle->curPagePos = ftell(pageFile); 
    fclose(pageFile); // Closing the file stream
    return RC_OK; // Success
}

extern RC readPreviousBlock(SM_FileHandle *fHandle, SM_PageHandle memPage) {
    // Checking if we are on the first block.
    if (fHandle->curPagePos <= PAGE_SIZE) {
        printf("\n First block: Previous block not present.");
        return RC_READ_NON_EXISTING_PAGE; // No previous block error
    }
    else {
        int currentPageNumber = fHandle->curPagePos / PAGE_SIZE; // Current page number
        int startPosition = (PAGE_SIZE * (currentPageNumber - 2)); // Start position

        // Opening file stream in read mode.
        pageFile = fopen(fHandle->fileName, "r");

        // Checking if file was successfully opened.
        if (pageFile == NULL)
            return RC_FILE_NOT_FOUND; // File not found error

        // Setting file pointer position.
        fseek(pageFile, startPosition, SEEK_SET);

        // Reading block character by character.
        for (int i = 0; i < PAGE_SIZE; i++) {
            memPage[i] = fgetc(pageFile); // Storing character
        }

        // Setting the current page position.
        fHandle->curPagePos = ftell(pageFile); 
        fclose(pageFile); // Closing the file stream
        return RC_OK; // Success
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
