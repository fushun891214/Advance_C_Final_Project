#ifndef COMMANDS_H
#define COMMANDS_H

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

#endif