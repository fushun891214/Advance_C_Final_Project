#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "vi.h"

#define INITIAL_CONTENT_SIZE 1024

void DisplayContentWithLineNumbers(const char* content) {
    printf("\nCurrent File Content:\n");
    int lineNumber = 1;
    const char* lineStart = content;
    while (*lineStart != '\0') {
        const char* lineEnd = strchr(lineStart, '\n');
        if (!lineEnd) lineEnd = lineStart + strlen(lineStart);

        printf("%3d | %.*s\n", lineNumber++, (int)(lineEnd - lineStart), lineStart);

        if (*lineEnd == '\0') break;
        lineStart = lineEnd + 1;
    }
    printf("-------------\n");
}

void ViEditorInteractive(INode* inode, char** content, size_t* contentSize) {
    int isSaved = 1;  // track if the file is saved

    while (1) {
        // 1) Display the file content
        DisplayContentWithLineNumbers(*content);

        // 2) Prompt user
        printf("Enter text or command (:i <n>, :d <n>, :w, :q, :wq, etc.)\n> ");

        // 3) Read user input
        char userInput[1024];
        if (!fgets(userInput, sizeof(userInput), stdin)) {
            printf("Error reading user input.\n");
            return;
        }
        // Remove trailing newline
        userInput[strcspn(userInput, "\n")] = '\0';

        // If user typed nothing, loop again
        if (strlen(userInput) == 0) {
            continue;
        }

        // 4) Look for colon (commands). If no colon, treat entire input as text
        char* colonPos = strchr(userInput, ':');
        if (!colonPos) {
            // No commands => treat entire line as text to append
            size_t newContentLen = strlen(*content) + strlen(userInput) + 2;
            if (newContentLen > *contentSize) {
                *contentSize = newContentLen + INITIAL_CONTENT_SIZE;
                *content = realloc(*content, *contentSize);
                if (!*content) {
                    printf("Memory allocation error.\n");
                    return;
                }
            }
            strcat(*content, userInput);
            strcat(*content, "\n");
            isSaved = 0;
            continue;
        }

        // We found a colon => everything before colon can be text
        // everything from colon onward can be a command
        size_t prefixLen = (size_t)(colonPos - userInput);
        char textBefore[1024];
        strncpy(textBefore, userInput, prefixLen);
        textBefore[prefixLen] = '\0';

        char commandPart[1024];
        strcpy(commandPart, colonPos); // e.g. ":i 3 more"

        // If there's text before the command, append it
        if (strlen(textBefore) > 0) {
            size_t newContentLen = strlen(*content) + strlen(textBefore) + 2;
            if (newContentLen > *contentSize) {
                *contentSize = newContentLen + INITIAL_CONTENT_SIZE;
                *content = realloc(*content, *contentSize);
                if (!*content) {
                    printf("Memory allocation error.\n");
                    return;
                }
            }
            strcat(*content, textBefore);
            strcat(*content, "\n");
            isSaved = 0;
        }

        // 5) Parse the commandPart
        // e.g. ":i 3", ":d 2", ":w", ":q", ":wq", etc.
        if (strcmp(commandPart, ":q") == 0) {
            // Quit
            if (!isSaved) {
                printf("You have unsaved changes. Quit anyway? (y/n): ");
                char c;
                if (scanf(" %c", &c) != 1) c = 'n';
                getchar(); // consume leftover newline
                if (c == 'y' || c == 'Y') {
                    printf("Exiting vi editor.\n");
                    return;
                }
                // else continue
            } else {
                printf("Exiting vi editor.\n");
                return;
            }
        }
        else if (strcmp(commandPart, ":w") == 0) {
            // Save
            WriteFileContent(inode, *content);
            printf("Changes saved.\n");
            isSaved = 1;
        }
        else if (strcmp(commandPart, ":wq") == 0) {
            // Save and quit
            WriteFileContent(inode, *content);
            printf("Saved. Exiting vi editor.\n");
            return;
        }
        else if (strncmp(commandPart, ":d", 2) == 0) {
            // :d <lineNumber>
            // Example: ":d 3"
            int lineNum = 0;
            // find space
            char* spacePos = strchr(commandPart, ' ');
            if (spacePos) {
                lineNum = atoi(spacePos + 1);
            }
            if (lineNum > 0) {
                DeleteLine(*content, lineNum);
                isSaved = 0;
            } else {
                printf("Invalid delete syntax (use :d <lineNumber>).\n");
            }
        }
        else if (strncmp(commandPart, ":i", 2) == 0) {
            // :i <lineNumber>
            // Insert after a line number
            int lineNum = 0;
            char* spacePos = strchr(commandPart, ' ');
            if (spacePos) {
                lineNum = atoi(spacePos + 1);
            }
            if (lineNum <= 0) {
                printf("Invalid insert syntax (use :i <lineNumber>).\n");
                continue;
            }

            // For demonstration, we can ask the user for text to insert
            // or parse from the same line, etc.
            // Here, let's ask the user on a separate prompt:
            printf("[Insert Mode] Enter the text to insert after line %d:\n> ", lineNum);
            char insertBuf[1024];
            if (!fgets(insertBuf, sizeof(insertBuf), stdin)) {
                printf("Error reading insert text.\n");
                continue;
            }
            // remove trailing newline
            insertBuf[strcspn(insertBuf, "\n")] = '\0';

            InsertAfterLine(content, contentSize, lineNum, insertBuf);
            isSaved = 0;
        }
        else {
            printf("Unrecognized command: '%s'\n", commandPart);
        }
    } // end while
}

void InsertAfterLine(char** content, size_t* contentSize, int lineNumber, const char* text) {
    int currentLine = 1;
    char* lineStart = *content;
    while (*lineStart != '\0') {
        char* lineEnd = strchr(lineStart, '\n');
        if (!lineEnd) lineEnd = lineStart + strlen(lineStart);

        if (currentLine == lineNumber) {
            // Insert after this line
            size_t oldLen = strlen(*content);
            size_t textLen = strlen(text);
            // +2 for possible extra newline + null terminator
            size_t newLen = oldLen + textLen + 2;  
            if (newLen > *contentSize) {
                *contentSize = newLen + INITIAL_CONTENT_SIZE;
                *content = realloc(*content, *contentSize);
                if (!*content) {
                    printf("Memory allocation error.\n");
                    return;
                }
            }

            // Copy from content start up to the end of "this line"
            // `lineEnd - content + 1` gives you index inclusive of the newline
            size_t prefixSize = (lineEnd - *content) + 1; 
            char* newBuf = malloc(newLen);
            if (!newBuf) {
                printf("Memory allocation error.\n");
                return;
            }
            strncpy(newBuf, *content, prefixSize);
            newBuf[prefixSize] = '\0';  // ensure termination

            // Add the new text
            strncat(newBuf, text, textLen);
            strcat(newBuf, "\n");

            // Add the remainder (everything after lineEnd)
            strncat(newBuf, lineEnd + 1, oldLen - prefixSize);

            // Copy back
            strcpy(*content, newBuf);
            free(newBuf);
            return;
        }

        if (*lineEnd == '\0') break;
        lineStart = lineEnd + 1;
        currentLine++;
    }

    // If we get here, line not found
    printf("Line number %d not found.\n", lineNumber);
}

void DeleteLine(char* content, int lineNumber) {
    // Find the line number
    int currentLine = 1;
    char* lineStart = content;
    while (*lineStart != '\0') {
        char* lineEnd = strchr(lineStart, '\n');
        if (!lineEnd) lineEnd = lineStart + strlen(lineStart);

        if (currentLine == lineNumber) {
            // Found the line number
            // Delete this line
            char* newContent = (char*)malloc(strlen(content) + 1);
            if (!newContent) {
                printf("Memory allocation error.\n");
                return;
            }

            // Copy content up to lineStart
            strncpy(newContent, content, (size_t)(lineStart - content));
            newContent[lineStart - content] = '\0';

            // Append the rest of the content
            strcat(newContent, lineEnd + 1);

            // Replace the content
            strcpy(content, newContent);
            free(newContent);
            return;
        }

        if (*lineEnd == '\0') break;
        lineStart = lineEnd + 1;
        currentLine++;
    }

    // If we reach here, the line number was not found
    printf("Line number %d not found.\n", lineNumber);
}

int SplitContent(char* content, char* lines[]) {
    int totalLines = 0;
    char* lineStart = content;
    while (*lineStart != '\0') {
        char* lineEnd = strchr(lineStart, '\n');
        if (!lineEnd) lineEnd = lineStart + strlen(lineStart);

        lines[totalLines] = (char*)malloc((size_t)(lineEnd - lineStart) + 1);
        if (!lines[totalLines]) {
            printf("Memory allocation error.\n");
            return totalLines;
        }

        strncpy(lines[totalLines], lineStart, (size_t)(lineEnd - lineStart));
        lines[totalLines][(size_t)(lineEnd - lineStart)] = '\0';

        totalLines++;

        if (*lineEnd == '\0') break;
        lineStart = lineEnd + 1;
    }

    return totalLines;
}

void FreeSplitLines(char* lines[], int totalLines) {
    for (int i = 0; i < totalLines; i++) {
        free(lines[i]);
    }
}

int SearchInode(const char* fullPath, INode* inodes) {
    for (int i = 0; i < sb->inodeCount; i++) {
        if (inodes[i].isUsed && strcmp(inodes[i].fileName, fullPath) == 0) {
            return i;
        }
    }
    return -1;
}

char* ReadFileContent(INode* inode) {
    char* content = (char*)malloc(inode->size + 1);
    if (!content) {
        printf("Memory allocation error.\n");
        return NULL;
    }

    int remainSize = inode->size;
    char* contentPos = content;

    for (int i = 0; i < 10 && remainSize > 0; i++) {
        if (inode->directBlocks[i] == -1) break;

        char* blockStart = virtualDisk + sb->firstDataBlock * BLOCKSIZE + inode->directBlocks[i] * BLOCKSIZE;
        int readSize = (remainSize > BLOCKSIZE) ? BLOCKSIZE : remainSize;
        memcpy(contentPos, blockStart, readSize);
        contentPos += readSize;
        remainSize -= readSize;
    }

    if (remainSize > 0 && inode->indirectBlock != -1) {
        int* indirectTable = (int*)(virtualDisk + sb->firstDataBlock * BLOCKSIZE + inode->indirectBlock * BLOCKSIZE);

        for (int i = 0; i < BLOCKSIZE / sizeof(int) && remainSize > 0; i++) {
            if (indirectTable[i] == -1) break;

            char* blockStart = virtualDisk + sb->firstDataBlock * BLOCKSIZE + indirectTable[i] * BLOCKSIZE;
            int readSize = (remainSize > BLOCKSIZE) ? BLOCKSIZE : remainSize;
            memcpy(contentPos, blockStart, readSize);
            contentPos += readSize;
            remainSize -= readSize;
        }
    }

    *contentPos = '\0';
    return content;
}

void WriteFileContent(INode* inode, const char* content) {
    int remainSize = strlen(content);
    char* contentPos = (char*)content;

    for (int i = 0; i < 10 && remainSize > 0; i++) {
        if (inode->directBlocks[i] == -1) {
            inode->directBlocks[i] = allocateBlock();
            if (inode->directBlocks[i] == -1) {
                printf("No free block available.\n");
                return;
            }
        }

        char* blockStart = virtualDisk + sb->firstDataBlock * BLOCKSIZE + inode->directBlocks[i] * BLOCKSIZE;
        int writeSize = (remainSize > BLOCKSIZE) ? BLOCKSIZE : remainSize;
        memcpy(blockStart, contentPos, writeSize);
        contentPos += writeSize;
        remainSize -= writeSize;
    }

    if (remainSize > 0) {
        if (inode->indirectBlock == -1) {
            inode->indirectBlock = allocateBlock();
            if (inode->indirectBlock == -1) {
                printf("No free block available.\n");
                return;
            }
        }

        int* indirectTable = (int*)(virtualDisk + sb->firstDataBlock * BLOCKSIZE + inode->indirectBlock * BLOCKSIZE);

        for (int i = 0; i < BLOCKSIZE / sizeof(int) && remainSize > 0; i++) {
            if (indirectTable[i] == -1) {
                indirectTable[i] = allocateBlock();
                if (indirectTable[i] == -1) {
                    printf("No free block available.\n");
                    return;
                }
            }

            char* blockStart = virtualDisk + sb->firstDataBlock * BLOCKSIZE + indirectTable[i] * BLOCKSIZE;
            int writeSize = (remainSize > BLOCKSIZE) ? BLOCKSIZE : remainSize;
            memcpy(blockStart, contentPos, writeSize);
            contentPos += writeSize;
            remainSize -= writeSize;
        }
    }

    inode->size = strlen(content);
    inode->modifyTime = time(NULL);
}

void InitializeNewInode(INode* inode, const char* fullPath) {
    strncpy(inode->fileName, fullPath, sizeof(inode->fileName) - 1);
    inode->fileName[sizeof(inode->fileName) - 1] = '\0'; // Ensure null termination
    inode->size = 0;
    inode->isUsed = 1;
    inode->fileType = 0;  // 0 indicates regular file
    inode->createTime = time(NULL);
    inode->modifyTime = time(NULL);

    for (int i = 0; i < 10; i++) {
        inode->directBlocks[i] = -1;
    }
    inode->indirectBlock = -1;
}