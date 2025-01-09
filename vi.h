#ifndef VI_H
#define VI_H

#include "space.h"

// Display the file content with line numbers
void DisplayContentWithLineNumbers(const char* content);
// The main vi-like editor function
void ViEditorInteractive(INode* inode, char** content, size_t* contentSize);
// Insert text after a specified line number
void InsertAfterLine(char** content, size_t* contentSize, int lineNumber, const char* text);
// Delete a specified line
void DeleteLine(char* content, int lineNumber);
// Split file content into lines (returns number of lines).
// `lines[]` should be big enough to hold all line pointers.
int SplitContent(char* content, char* lines[]);
// Free the array of lines (when `SplitContent` uses malloc-ed copies).
void FreeSplitLines(char* lines[], int totalLines);
// Search for an inode by full path
int SearchInode(const char* fullPath, INode* inodes);
// Read file content from an inode
char* ReadFileContent(INode* inode);
// Write file content to an inode
void WriteFileContent(INode* inode, const char* content);
// Initialize a new inode (like "touch" or new file creation).
void InitializeNewInode(INode* inode, const char* fullPath);

#endif
