#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "vi.h"

void DisplayContentWithLineNumbers(const char* content) {
    int lineNumber = 1;
    const char* lineStart = content;
    printf("\nCurrent File Content:\n");
    while (*lineStart != '\0') {
        const char* lineEnd = strchr(lineStart, '\n');
        if (!lineEnd) lineEnd = lineStart + strlen(lineStart);

        printf("%3d | %.*s\n", lineNumber++, (int)(lineEnd - lineStart), lineStart);

        if (*lineEnd == '\0') break;
        lineStart = lineEnd + 1;
    }
    printf("-------------\n");
}

void ViEditorInteractive(INode* inode, char* content) {
    printf("\nEntering vi editor mode. Enter :q to return to the main menu.");
    char command[10];
    int isSaved = 1; // Track if the file is saved
    while(getchar() != '\n');

    while (1) {
        DisplayContentWithLineNumbers(content);

        printf("Normal Mode. Commands: :i (insert), :d (delete), :w (save), :q (exit).\n: ");
        fgets(command, sizeof(command), stdin);

        if (strcmp(command, ":i\n") == 0) {
            printf("Enter the line number to insert after: ");
            int lineNumber;
            if (scanf("%d", &lineNumber) != 1) {
                printf("Invalid input. Returning to normal mode.\n");
                getchar(); // Consume leftover input
                continue;
            }
            getchar(); // Consume newline
            InsertMode(content, lineNumber);
            isSaved = 0; // Mark as unsaved
        } else if (strcmp(command, ":d\n") == 0) {
            printf("Enter the line number to delete: ");
            int lineNumber;
            if (scanf("%d", &lineNumber) != 1) {
                printf("Invalid input. Returning to normal mode.\n");
                getchar(); // Consume leftover input
                continue;
            }
            getchar(); // Consume newline
            DeleteLine(content, lineNumber);
            DisplayContentWithLineNumbers(content); // Show updated content immediately
            isSaved = 0; // Mark as unsaved
        } else if (strcmp(command, ":w\n") == 0) {
            WriteFileContent(inode, content);
            printf("Changes saved.\n");
            isSaved = 1; // Mark as saved
        } else if (strcmp(command, ":q\n") == 0) {
            if (!isSaved) {
                printf("You have unsaved changes. Do you really want to exit? (y/n): ");
                char confirm;
                scanf(" %c", &confirm);
                getchar(); // Consume newline
                if (confirm == 'y' || confirm == 'Y') {
                    printf("Exiting vi editor. Returning to command mode.\n");
                    return;
                } else {
                    printf("Returning to normal mode.\n");
                    continue;
                }
            }
            printf("Exiting vi editor. Returning to command mode.\n");
            return;
        } else {
            printf("Invalid command. Available commands: :i, :d, :w, :q\n");
        }
    }
}

void InsertMode(char* content, int lineNumber) {
    printf("Entering insert mode at line %d. Type your content below.\n(Type ':q' on a new line to quit insert mode.)\n", lineNumber);
    char line[256];

    // Insert logic: split content at lineNumber, insert each line as it is typed, and reassemble
    char* lines[1024];
    int totalLines = SplitContent(content, lines);

    while (1) {
        DisplayContentWithLineNumbers(content);
        printf("> ");
        fgets(line, sizeof(line), stdin);
        if (strcmp(line, ":q\n") == 0) break;

        // Create updated content by inserting the new line
        char* updatedContent = (char*)malloc(strlen(content) + strlen(line) + 1);
        if (!updatedContent) {
            fprintf(stderr, "Memory allocation failed for updatedContent.\n");
            return;
        }

        strcpy(updatedContent, "");
        for (int i = 0; i < lineNumber; i++) {
            strcat(updatedContent, lines[i]);
            strcat(updatedContent, "\n");
        }
        strcat(updatedContent, line);
        if (lineNumber < totalLines) {
            for (int i = lineNumber; i < totalLines; i++) {
                strcat(updatedContent, lines[i]);
                strcat(updatedContent, "\n");
            }
        }
        strcpy(content, updatedContent);
        free(updatedContent);

        // Update split lines to reflect changes
        totalLines = SplitContent(content, lines);
    }

    printf("Exiting insert mode. Returning to normal mode.\n");
}

void DeleteLine(char* content, int lineNumber) {
    printf("Deleting line %d.\n", lineNumber);
    // Deletion logic: split content, remove the line, and reassemble
    char* lines[1024];
    int totalLines = SplitContent(content, lines);

    if (lineNumber > 0 && lineNumber <= totalLines) {
        char* updatedContent = (char*)malloc(strlen(content) + 1);
        if (!updatedContent) {
            fprintf(stderr, "Memory allocation failed for updatedContent.\n");
            return;
        }

        strcpy(updatedContent, "");
        for (int i = 0; i < totalLines; i++) {
            if (i != lineNumber - 1) {
                strcat(updatedContent, lines[i]);
                strcat(updatedContent, "\n");
            }
        }
        strcpy(content, updatedContent);
        free(updatedContent);
    } else {
        printf("Invalid line number. Returning to normal mode.\n");
    }

    printf("Line deleted. Returning to normal mode.\n");
}

int SplitContent(char* content, char* lines[]) {
    char* tempContent = strdup(content); // Duplicate content to avoid modifying the original
    if (!tempContent) {
        fprintf(stderr, "Memory allocation failed for tempContent.\n");
        return 0;
    }

    int lineCount = 0;
    char* line = strtok(tempContent, "\n");
    while (line != NULL) {
        lines[lineCount++] = strdup(line); // Allocate memory for each line
        line = strtok(NULL, "\n");
    }

    free(tempContent);
    return lineCount;
}

void FreeSplitLines(char* lines[], int totalLines) {
    for (int i = 0; i < totalLines; i++) {
        free(lines[i]); // Free each allocated line
    }
}

void WriteFileContent(INode* inode, const char* content) {
    int contentLength = strlen(content);
    int numBlocks = (contentLength + BLOCKSIZE - 1) / BLOCKSIZE;

    // Allocate direct blocks and write content
    for (int i = 0; i < numBlocks && i < 10; i++) {
        int blockNum = allocateBlock();
        if (blockNum == -1) {
            printf("No free blocks available\n");
            return;
        }

        inode->directBlocks[i] = blockNum;
        char* blockStart = virtualDisk + sb->firstDataBlock * BLOCKSIZE + blockNum * BLOCKSIZE;

        int writeSize = ((i + 1) * BLOCKSIZE > contentLength) ? (contentLength - i * BLOCKSIZE) : BLOCKSIZE;
        memcpy(blockStart, content + i * BLOCKSIZE, writeSize);
    }

    // Update inode information
    inode->size = contentLength;
    inode->modifyTime = time(NULL);
}


// ViEditor
int SearchInode(const char* fullPath, INode* inodes) {
    for (int i = 0; i < sb->inodeCount; i++) {
        if (inodes[i].isUsed && strcmp(inodes[i].fileName, fullPath) == 0) {
            return i;
        }
    }
    return -1;
}

char* ReadFileContent(INode* inode) {
    int remainSize = inode->size;
    char* content = (char*)malloc(inode->size + 1);
    if (!content) return NULL;

    char* contentPtr = content;
    for (int i = 0; i < 10 && remainSize > 0; i++) {
        if (inode->directBlocks[i] == -1) break;

        char* blockStart = virtualDisk + sb->firstDataBlock * BLOCKSIZE + inode->directBlocks[i] * BLOCKSIZE;
        int readSize = (remainSize > BLOCKSIZE) ? BLOCKSIZE : remainSize;
        memcpy(contentPtr, blockStart, readSize);
        contentPtr += readSize;
        remainSize -= readSize;
    }

    if (remainSize > 0 && inode->indirectBlock != -1) {
        int* indirectTable = (int*)(virtualDisk + sb->firstDataBlock * BLOCKSIZE + inode->indirectBlock * BLOCKSIZE);

        for (int i = 0; i < BLOCKSIZE / sizeof(int) && remainSize > 0; i++) {
            if (indirectTable[i] == -1) break;

            char* blockStart = virtualDisk + sb->firstDataBlock * BLOCKSIZE + indirectTable[i] * BLOCKSIZE;
            int readSize = (remainSize > BLOCKSIZE) ? BLOCKSIZE : remainSize;
            memcpy(contentPtr, blockStart, readSize);
            contentPtr += readSize;
            remainSize -= readSize;
        }
    }

    content[inode->size] = '\0';
    return content;
}

void InitializeNewInode(INode* inode, const char* fullPath) {
    strncpy(inode->fileName, fullPath, sizeof(inode->fileName) - 1);
    inode->size = 0;
    inode->isUsed = 1;
    inode->fileType = 0;  // Regular file
    inode->createTime = time(NULL);
    inode->modifyTime = time(NULL);

    for (int i = 0; i < 10; i++) inode->directBlocks[i] = -1;
    inode->indirectBlock = -1;
}