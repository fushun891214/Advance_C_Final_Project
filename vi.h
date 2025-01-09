#ifndef VI_H
#define VI_H

#include "space.h"

void DisplayContentWithLineNumbers(const char* content);
void ViEditorInteractive(INode* inode, char* content);
void InsertMode(char* content, int lineNumber);
void DeleteLine(char* content, int lineNumber);
int SplitContent(char* content, char* lines[]);
void FreeSplitLines(char* lines[], int totalLines);
int SearchInode(const char* fullPath, INode* inodes);
char* ReadFileContent(INode* inode);
void WriteFileContent(INode* inode, const char* content);
void InitializeNewInode(INode* inode, const char* fullPath);


#endif
