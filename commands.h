#ifndef COMMANDS_H
#define COMMANDS_H
#define MAX_PATH_LEN 256
#include "space.h"

extern char currentPath[MAX_PATH_LEN];
extern INode* currentDir;

void ListFiles(void);
void ChangeDirectory(char *path);
void RemoveFile(char *path);
void MakeDirectory(char *path);
void RemoveDirectory(char *path);
int PutFile(char *path);
int GetFile(char *path);
int ViEditor(char *path);
void DisplayFileContent(char *path);
void DisplayStatus();
void Help(void);
void ExitAndStoreImage(void);
void LoadDumpImage(void);
int ExportDirectory(char *path, INode* dirInode); 

#endif