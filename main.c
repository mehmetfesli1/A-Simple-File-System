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
// Signed: Mehmet Fesli Date:_____________

//filesys.c
//Based on a program by Michael Black, 2007
//Revised 11.3.2020 O'Neil

#include <stdio.h>
#include <string.h>

// CONSTANTS for the filesystem
#define MAX_LENGTH_FILENAME 8
#define TEXT_FILE 't'
#define EXEC_FILE 'x'
#define DIRECTORY_SECTOR 257
#define MAP_SECTOR 256
#define SECTOR_SIZE 512
#define MAX_BUFFER_SIZE 12288 // Maximum file size (24 sectors)

// Function prototypes
void listFiles(char* map, char* dir);
void printFile(char* map, char* dir, char* filename, FILE* floppy);
void makeFile(char* map, char* dir, char* filename, FILE* floppy);
void deleteFile(char* map, char* dir, char* filename, FILE* floppy);
void writeBackToDisk(char* map, char* dir, FILE* floppy);


int main(int argc, char* argv[])
{
	int i, j, size, noSecs, startPos;
    char filename[MAX_LENGTH_FILENAME + 1]; // +1 for null terminator

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

    char option = argv[1][0];  // First character of the first argument

    // Yields an error message for any other option
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
        // Copy filename from command line
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


    // switch statement to choose which function run
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
    
    // Go through directory entries
    for (i = 0; i < SECTOR_SIZE; i += 16) {
        if (dir[i] == 0) continue; // Hopefully skips empty entries
        
        // to extract and print filename in proper format
        for (j = 0; j < MAX_LENGTH_FILENAME; j++) {
            if (dir[i + j] == 0) break;
            filename[j] = dir[i + j];
        }
        filename[j] = '\0';
        
        // Get file type (for extension)
        extension = (dir[i + 8] == TEXT_FILE) ? 't' : 'x';
        
        // Calculate file size in bytes
        int fileSize = 512 * dir[i + 10];
        totalUsed += fileSize;
        totalFiles++;
        
        printf("%s.%c      %d\n", filename, extension, fileSize);
    }
    
    // Calculate and print total space statistics
    printf("\nTotal files: %d\n", totalFiles);
    printf("Space used: %d bytes\n", totalUsed);
    printf("Space free: %d bytes\n", 261632 - totalUsed);
}


void printFile(char* map, char* dir, char* filename, FILE* floppy){

    int i, j;
    int fileFound = 0;
    int startSector = 0;
    int sectorCount = 0;
    char fileType;
    char buffer[MAX_BUFFER_SIZE];
    
    // Search for file in directory
    for (i = 0; i < SECTOR_SIZE; i += 16) {
        if (dir[i] == 0) continue; // Skip empty entries
        // Check if filename matches
        int match = 1;
        for (j = 0; j < MAX_LENGTH_FILENAME && filename[j] != '\0'; j++) {
            if (dir[i + j] != filename[j]) {
                match = 0;
                break;
            }
        }
        // If we've matched so far, make sure we've reached the end of the filename
        // This is a failscheck to prevent the code to match a file with partial input.
        // Before this check, it used to print out the contents of msg when I just inputted 'm'
        // Now it verifies the remainder of the directory entry name is zeros.
        if (match && filename[j] == '\0') {
            // Verify remainder of directory entry name field is just zeros
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
    } if (fileFound == 0) {
        printf("Error: File not found.\n");
        return;
    } if (fileType != TEXT_FILE) {
        printf("Error: Cannot print non-text file.\n");
        return;
    }
    
    // Load the file contents to the buffer
    int fileSize = sectorCount * SECTOR_SIZE;
    fseek(floppy, SECTOR_SIZE * startSector, SEEK_SET);
    for (i = 0; i < fileSize && i < MAX_BUFFER_SIZE; i++) {
        buffer[i] = fgetc(floppy);
    }
    
    // Must print the file contents until the null terminator
    printf("File contents:\n");
    printf("-------------\n");
    for (i = 0; i < fileSize && buffer[i] != 0; i++) {
        putchar(buffer[i]);
    }
    printf("\n");
}


void makeFile(char* map, char* dir, char* filename, FILE* floppy){



}

// Definition for D option
void deleteFile(char* map, char* dir, char* filename, FILE* floppy){



}

// writes back to the disk
void writeBackToDisk(char* map, char* dir, FILE* floppy){



}




// {



//     //print disk map
// 	printf("Disk usage map:\n");
// 	printf("      0 1 2 3 4 5 6 7 8 9 A B C D E F\n");
// 	printf("     --------------------------------\n");
// 	for (i=0; i<16; i++) {
// 		switch(i) {
// 			case 15: printf("0xF_ "); break;
// 			case 14: printf("0xE_ "); break;
// 			case 13: printf("0xD_ "); break;
// 			case 12: printf("0xC_ "); break;
// 			case 11: printf("0xB_ "); break;
// 			case 10: printf("0xA_ "); break;
// 			default: printf("0x%d_ ", i); break;
// 		}
// 		for (j=0; j<16; j++) {
// 			if (map[16*i+j]==-1) printf(" X"); else printf(" .");
// 		}
// 		printf("\n");
// 	}

//     // print directory
// 	printf("\nDisk directory:\n");
// 	printf("Name    Type Start Length\n");
//     for (i=0; i<512; i=i+16) {
// 		if (dir[i]==0) break;
// 		for (j=0; j<8; j++) {
// 			if (dir[i+j]==0) printf(" "); else printf("%c",dir[i+j]);
// 		}
// 		if ((dir[i+8]=='t') || (dir[i+8]=='T')) printf("text"); else printf("exec");
// 		printf(" %5d %6d bytes\n", dir[i+9], 512*dir[i+10]);
// 	}



// 	//write the map and directory back to the floppy image
//     fseek(floppy,512*256,SEEK_SET);
//     for (i=0; i<512; i++) fputc(map[i],floppy);

//     fseek(floppy,512*257,SEEK_SET);
//     for (i=0; i<512; i++) fputc(dir[i],floppy);

// 	fclose(floppy);


//     // switch (option) {
//     //     case 'L':
//     //         // Print directory listing
//     //         printf("Disk directory:\n");
//     //         printf("Name        Size\n");
            
//     //         int totalUsed = 0;

            
//     //         for (i=0; i<512; i=i+16) {

//     //             // skip empty entries
//     //             if (dir[i]==0) continue;

//     //             // prints filename
//     //             for (j=0; j<8; j++) {
//     //                 if (dir[i+j]==0) printf(" "); else printf("%c",dir[i+j]);
//     //             }









// }
