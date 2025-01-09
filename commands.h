#ifndef COMMANDS_H
#define COMMANDS_H

#include "space.h"

#define MAX_PATH_LEN 512

extern char currentPath[MAX_PATH_LEN];
extern INode* currentDir;

void HandleCommands(void);
void ListFiles(void);
void ChangeDirectory(char *path);
void RemoveFile(char *path);
void MakeDirectory(char *path);
void RemoveDirectory(char *path);
int PutFile(char *path);
int GetFile(char *path);
int ViEditor(char *path);
void ConstructFullPath(char* fullPath, const char* path);
void DisplayFileContent(char *path);
void DisplayStatus(void);
void Help(void);
void ExitAndStoreImage(void);
int LoadDumpImage(char *path);
int ExportDirectory(char *path, INode* dirInode);

void EncryptVirtualDisk(char* virtualDisk, int partitionSize, char* password, int passwordLength);
void DecryptVirtualDisk(char* virtualDisk, int partitionSize, char* password, int passwordLength);

#endif
