// ACADEMIC INTEGRITY PLEDGE
//
// - I have not used source code obtained from another student nor
//   any other unauthorized source, either modified or unmodified.
//
// - All source code and documentation used in my program is either
//   my original work or was derived by me from the source code
//   published in the textbook for this course or presentged in
//   class.
//
// - I have not discussed coding details about this project with
//   anyone other than my instructor. I understand that I may discuss
//   the concepts of this program with other students and that another
//   student may help me debug my program so long as neither of us
//   writes anything during the discussion or modifies any computer
//   file during the discussion.
//
// - I have violated neither the spirit nor letter of these restrictions.
//
//
//
// Signed: Mehmet Fesli Date: 05/01/2025

//filesys.c
//Based on a program by Michael Black, 2007
//Revised 11.3.2020 O'Neil

#include <stdio.h>
#include <string.h>

// Just defining some constants to use - makes the code cleaner!
#define MAX_LENGTH_FILENAME 8
#define TEXT_FILE 't'
#define EXEC_FILE 'x'
#define DIRECTORY_SECTOR 257
#define MAP_SECTOR 256
#define SECTOR_SIZE 512
#define MAX_BUFFER_SIZE 12288 // Max file size (24 sectors)

// Functions I need to implement
void listFiles(char* map, char* dir);
void printFile(char* map, char* dir, char* filename, FILE* floppy);
void makeFile(char* map, char* dir, char* filename, FILE* floppy);
void deleteFile(char* map, char* dir, char* filename, FILE* floppy);
void writeBackToDisk(char* map, char* dir, FILE* floppy); // helper function


int main(int argc, char* argv[])
{
	int i, j, size, noSecs, startPos;
    char filename[MAX_LENGTH_FILENAME + 1]; // +1 for null terminator

    // Menu display for the user know the list of options they can run
    if (argc < 2) {
        printf("\n╔═══════════════════════════════════════════════════════════╗\n");
        printf("║     🌟 WELCOME TO THE RETRO FLOPPY FILE SYSTEM! 🌟        ║\n");
        printf("╠═══════════════════════════════════════════════════════════╣\n");
        printf("║  Available Commands:                                      ║\n");
        printf("║                                                           ║\n");
        printf("║  📋 ./filesys L                                           ║\n");
        printf("║     Show all files on your virtual floppy                 ║\n");
        printf("║                                                           ║\n");
        printf("║  📖 ./filesys P filename                                  ║\n");
        printf("║     Read the contents of a text file                      ║\n");
        printf("║                                                           ║\n");
        printf("║  ✏️  ./filesys M filename                                 ║\n");
        printf("║     Create a new text file on the floppy                  ║\n");
        printf("║                                                           ║\n");
        printf("║  🗑️  ./filesys D filename                                 ║\n");
        printf("║     Delete a file from the floppy                         ║\n");
        printf("╚═══════════════════════════════════════════════════════════╝\n");
        return 1;
    }

    char option = argv[1][0];  // Get first char of first arg

    // Error messages for bad options
    if (option != 'L' && option != 'P' && option != 'M' && option != 'D') {
        printf("\n⚠️  Command not recognized! Please use L, P, M, or D. ⚠️\n");
        printf("Type './filesys' without parameters to see all available commands.\n\n");
        return 1;
    }
    if (option != 'L' && argc < 3) {
        printf("\n⚠️  ERROR: Entered Command requries a filename. Please reenter the command with the appropriate filename. ⚠️\n");
        return 1;

    }

    if (option != 'L') {
        // Copy the filename from command prompt
        strncpy(filename, argv[2], MAX_LENGTH_FILENAME);
        filename[MAX_LENGTH_FILENAME] = '\0'; // Ensure null termination
    }


	//open the floppy image
	FILE* floppy;
	floppy=fopen("floppya.img","r+");
	if (floppy==0)
	{
		printf("floppya.img not found\n");
		return 0;
	}

	//load the disk map from sector 256
	char map[512];
	fseek(floppy,512*256,SEEK_SET);
	for(i=0; i<512; i++)
		map[i]=fgetc(floppy);

	//load the directory from sector 257
	char dir[512];
	fseek(floppy,512*257,SEEK_SET);
	for (i=0; i<512; i++)
		dir[i]=fgetc(floppy);


    // figure out which command to run
    switch (option) {
        case 'L':
            listFiles(map, dir);
            break;
        case 'P':
            printFile(map, dir, filename, floppy);
            break;
        case 'M':
            makeFile(map, dir, filename, floppy);
            break;
        case 'D':
            deleteFile(map, dir, filename, floppy);
            break;
    }

    fclose(floppy);

    return 0;

}


// Function Definitons

void listFiles(char* map, char* dir){
    int i, j;
    int totalUsed = 0;
    int totalFiles = 0;
    char filename[MAX_LENGTH_FILENAME + 1];
    char extension;
    
    printf("Files on disk:\n");
    printf("Name      Size (bytes)\n");
    printf("--------------------\n");
    
    // Go thru directory entries
    for (i = 0; i < SECTOR_SIZE; i += 16) {
        if (dir[i] == 0) continue; // skip empty entries
        
        // extract and print filename in 8.3 format
        for (j = 0; j < MAX_LENGTH_FILENAME; j++) {
            if (dir[i + j] == 0) break;
            filename[j] = dir[i + j];
        }
        filename[j] = '\0';
        
        // figure out file type for extension
        extension = (dir[i + 8] == TEXT_FILE) ? 't' : 'x';
        
        // calculate file size & update totals
        int fileSize = 512 * dir[i + 10];
        totalUsed += fileSize;
        totalFiles++;
        
        printf("%s.%c      %d\n", filename, extension, fileSize);
    }
    
    // Print summary info
    printf("\nTotal files: %d\n", totalFiles);
    printf("Space used: %d bytes\n", totalUsed);
    printf("Space free: %d bytes\n", 261632 - totalUsed); // total space is 512 sectors - 1
}


void printFile(char* map, char* dir, char* filename, FILE* floppy){

    int i, j;
    int fileFound = 0;
    int startSector = 0;
    int sectorCount = 0;
    char fileType;
    char buffer[MAX_BUFFER_SIZE];
    
    // Look for the file in directory
    for (i = 0; i < SECTOR_SIZE; i += 16) {
        if (dir[i] == 0) continue; // skip empty entries
        
        // check if filename matches
        int match = 1;
        for (j = 0; j < MAX_LENGTH_FILENAME && filename[j] != '\0'; j++) {
            if (dir[i + j] != filename[j]) {
                match = 0;
                break;
            }
        }
        
        // Fixed the partial match bug - need exact match!
        // Had a weird issue where typing "m" would print contents of "msg"
        // Now it makes sure the input is matched with the file name before printing its contents
        if (match && filename[j] == '\0') {
            // Make sure rest of name field is zeros
            for (; j < MAX_LENGTH_FILENAME; j++) {
                if (dir[i + j] != 0) {
                    match = 0;
                    break;
                }
            }
        }
        
        if (match) {
            fileFound = 1;
            fileType = dir[i + 8];
            startSector = dir[i + 9];
            sectorCount = dir[i + 10];
            break;
        }
    } 
    
    // handle errors
    if (fileFound == 0) {
        printf("Error: File not found.\n");
        return;
    } 
    if (fileType != TEXT_FILE) {
        printf("Error: Cannot print non-text file.\n");
        return;
    }
    
    // Load file contents into buffer
    int fileSize = sectorCount * SECTOR_SIZE;
    fseek(floppy, SECTOR_SIZE * startSector, SEEK_SET);
    for (i = 0; i < fileSize && i < MAX_BUFFER_SIZE; i++) {
        buffer[i] = fgetc(floppy);
    }
    
    // Print the contents
    printf("File contents:\n");
    printf("-------------\n");
    for (i = 0; i < fileSize && buffer[i] != 0; i++) {
        putchar(buffer[i]);
    }
    printf("\n");
}


void makeFile(char* map, char* dir, char* filename, FILE* floppy){
    int i;
    int freeEntry = -1; // no free entry found yet
    int freeSector = -1; // no free sector found yet
    char buffer[SECTOR_SIZE];
    char trimmedName[MAX_LENGTH_FILENAME + 1];
    
    // Trim filename to 8 chars max (DOS style)
    memset(trimmedName, 0, MAX_LENGTH_FILENAME + 1);
    strncpy(trimmedName, filename, MAX_LENGTH_FILENAME);
    
    // Check for duplicates & find empty spot
    for (i = 0; i < SECTOR_SIZE; i += 16) {
        // Empty dir entry? save it
        if (dir[i] == 0) {
            if (freeEntry == -1) freeEntry = i;
            continue;
        }
        
        // check if file exists already
        char entryName[MAX_LENGTH_FILENAME + 1] = {0};
        int j;
        for (j = 0; j < MAX_LENGTH_FILENAME && dir[i + j] != 0; j++) {
            entryName[j] = dir[i + j];
        }
        
        if (strcmp(entryName, trimmedName) == 0) {
            printf("\n!! Oops! A file named '%s' already exists!\n", trimmedName);
            printf("   Try a different name.\n\n");
            return;
        }
    }
    
    // no empty dir slots? display error
    if (freeEntry == -1) {
        printf("\n!! Directory is full! Delete some files first.\n\n");
        return;
    }
    
    // Find a free sector to use
    for (i = 0; i < SECTOR_SIZE; i++) {
        if (map[i] == 0) {  // 0 = free sector
            freeSector = i;
            break;
        }
    }
    
    if (freeSector == -1) {
        printf("\n!! Disk is full! Delete some files to make space.\n\n");
        return;
    }
    
    // Get user input for file content
    printf("\n Creating new file: %s.t\n", trimmedName);
    printf("╔═════════════════════════════════════════════╗\n");
    printf("║ Type your text below (press Enter when done)║\n");
    printf("╚═════════════════════════════════════════════╝\n");
    printf("▶ ");
    
    // Clear buffer & get input
    memset(buffer, 0, SECTOR_SIZE);
    if (fgets(buffer, SECTOR_SIZE, stdin) == NULL) {
        printf("\n Couldn't read your input!\n\n");
        return;
    }
    
    // get rid of the newline that fgets adds
    size_t len = strcspn(buffer, "\n");
    if (len < SECTOR_SIZE) {
        buffer[len] = 0;
    }
    
    // create the dir entry
    memset(&dir[freeEntry], 0, 16);
    strncpy(&dir[freeEntry], trimmedName, MAX_LENGTH_FILENAME);
    dir[freeEntry + 8] = TEXT_FILE;  // t for text file
    dir[freeEntry + 9] = freeSector; // where it starts
    dir[freeEntry + 10] = 1;         // use 1 sector
    
    // mark sector as used
    map[freeSector] = -1;  // -1 = FF = used
    
    // write file to disk
    fseek(floppy, SECTOR_SIZE * freeSector, SEEK_SET);
    fwrite(buffer, 1, SECTOR_SIZE, floppy);
    
    // update directory and map
    writeBackToDisk(map, dir, floppy);
    
    size_t contentLength = strlen(buffer);
    printf("\n✅ File '%s.t' created with %zu bytes.\n\n", trimmedName, contentLength);
}

// Function for D option - to delete a file
void deleteFile(char* map, char* dir, char* filename, FILE* floppy){
    int i, j;
    int fileFound = 0;
    int dirEntry = -1;
    int startSector = 0;
    int sectorCount = 0;
    char fileType;
    char fullname[MAX_LENGTH_FILENAME + 3]; // +3 for extension & null
    
    // Find the file
    for (i = 0; i < SECTOR_SIZE; i += 16) {
        if (dir[i] == 0) continue; // skip empty entries
        
        // check if filename matches
        int match = 1;
        for (j = 0; j < MAX_LENGTH_FILENAME && filename[j] != '\0'; j++) {
            if (dir[i + j] != filename[j]) {
                match = 0;
                break;
            }
        }
        
        // same fix as in printFile - need exact match
        if (match && filename[j] == '\0') {
            for (; j < MAX_LENGTH_FILENAME; j++) {
                if (dir[i + j] != 0) {
                    match = 0;
                    break;
                }
            }
        }
        
        if (match) {
            fileFound = 1;
            dirEntry = i;
            fileType = dir[i + 8];
            startSector = dir[i + 9];
            sectorCount = dir[i + 10];
            break;
        }
    }
    
    if (!fileFound) {
        printf("\n!! Cannot delete '%s': File not found on this floppy disk!\n\n", filename);
        return;
    }
    
    // Build full filename with extension for display
    strncpy(fullname, filename, MAX_LENGTH_FILENAME);
    fullname[MAX_LENGTH_FILENAME] = '.';
    fullname[MAX_LENGTH_FILENAME + 1] = (fileType == TEXT_FILE) ? 't' : 'x';
    fullname[MAX_LENGTH_FILENAME + 2] = '\0';
    
    // Double-check with user
    char confirm;
    printf("\n!! WARNING: You are about to delete '%s'!\n", fullname);
    printf("   This will permanently remove the file from the disk.\n");
    printf("   Continue? (y/n): ");
    scanf(" %c", &confirm);
    
    if (confirm != 'y' && confirm != 'Y') {
        printf("\n!! Deletion cancelled. Your file is safe!\n\n");
        return;
    }
    
    // Empty the directory entry - just set first byte to 0
    dir[dirEntry] = 0;
    
    // Free up disk sectors
    for (i = 0; i < sectorCount; i++) {
        map[startSector + i] = 0;  // mark as free
    }
    
    // Save changes
    writeBackToDisk(map, dir, floppy);
    
    printf("\n!! Success! File '%s' has been deleted.\n", fullname);
    printf("   %d sectors (%d bytes) have been freed up.\n\n", 
           sectorCount, sectorCount * SECTOR_SIZE);
}

// Helper function to write changes back to disk
// Saves typing the same code over and over
void writeBackToDisk(char* map, char* dir, FILE* floppy){
    int i;
    
    // Save map back to sector 256
    fseek(floppy, SECTOR_SIZE * MAP_SECTOR, SEEK_SET);
    for (i = 0; i < SECTOR_SIZE; i++)fputc(map[i], floppy);
    
    // Save directory back to sector 257
    fseek(floppy, SECTOR_SIZE * DIRECTORY_SECTOR, SEEK_SET);
    for (i = 0; i < SECTOR_SIZE; i++) fputc(dir[i], floppy);
}