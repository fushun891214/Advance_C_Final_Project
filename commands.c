#include "commands.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "space.h"

// 列出所有檔案
void ListFiles(void){
    INode* inodes = (INode*)(virtualDisk + sizeof(SuperBlock));
    printf("\n檔案列表:\n");
    printf("---------------------------------------------------------\n");
    printf("檔名\t\t大小\t\t建立時間\n");
    printf("---------------------------------------------------------\n");
    
    for (int i = 0; i < sb->inodeCount; i++) {
        if (inodes[i].isUsed) {
            char timeStr[26];
            ctime_r(&inodes[i].createTime, timeStr);
            timeStr[24] = '\0';  // 移除換行符
            printf("%-15s\t%d bytes\t\t%s\n", 
                   inodes[i].fileName, 
                   inodes[i].size, 
                   timeStr);
        }
    }
    printf("---------------------------------------------------------\n");
}

void ChangeDirectory(char *path){  // cd
    INode* inodes = (INode*)(virtualDisk + sizeof(SuperBlock));
    int foundInodeIndex = -1;
    
    for(int i = 0; i < sb->inodeCount; i++) {
        if(inodes[i].isUsed && strcmp(inodes[i].fileName, path) == 0) {
            foundInodeIndex = i;
            break;
        }
    }
    
    if(foundInodeIndex == -1) {
        printf("Directory '%s' not found\n", path);
        return;
    }
    
    INode* inode = &inodes[foundInodeIndex];
    if(inode->fileType != 1) {
        printf("'%s' is not a directory\n", path);
        return;
    }
    
    printf("Directory changed to '%s'\n", path);
}

void RemoveFile(char *path) {
    // 1. 檢查並找到檔案的 inode
    INode* inodes = (INode*)(virtualDisk + sizeof(SuperBlock));
    int foundInodeIndex = -1;
    
    for(int i = 0; i < sb->inodeCount; i++) {
        if(inodes[i].isUsed && strcmp(inodes[i].fileName, path) == 0) {
            foundInodeIndex = i;
            break;
        }
    }
    
    if(foundInodeIndex == -1) {
        printf("File '%s' not found\n", path);
        return;
    }

    // 2. 釋放所有直接區塊
    INode* inode = &inodes[foundInodeIndex];
    for(int i = 0; i < 10; i++) {
        if(inode->directBlocks[i] != -1) {
            freeBlock(inode->directBlocks[i]);
            inode->directBlocks[i] = -1;
        }
    }

    // 3. 處理間接區塊（如果有）
    if(inode->indirectBlock != -1) {
        // 取得間接區塊表
        int* indirectTable = (int*)(virtualDisk + 
                                  sb->firstDataBlock * BLOCKSIZE + 
                                  inode->indirectBlock * BLOCKSIZE);
        
        // 釋放所有指向的區塊
        for(int i = 0; i < BLOCKSIZE/sizeof(int); i++) {
            if(indirectTable[i] != -1) {
                freeBlock(indirectTable[i]);
            }
        }
        // 釋放間接區塊本身
        freeBlock(inode->indirectBlock);
        inode->indirectBlock = -1;
    }

    // 4. 更新 inode 資訊
    inode->isUsed = 0;
    inode->size = 0;
    memset(inode->fileName, 0, sizeof(inode->fileName));
    
    // 5. 更新系統計數器
    sb->freeInodeCount++;
    sb->usedInodeCount--;

    printf("File '%s' removed successfully\n", path);
}

// void RemoveFile(char *path){ // rm
//     INode* inodes = (INode*)(virtualDisk + sizeof(SuperBlock));
//     int foundInodeIndex = -1;
    
//     for(int i = 0; i < sb->inodeCount; i++) {
//         if(inodes[i].isUsed && strcmp(inodes[i].fileName, path) == 0) {
//             foundInodeIndex = i;
//             break;
//         }
//     }
    
//     if(foundInodeIndex == -1) {
//         printf("File '%s' not found\n", path);
//         return;
//     }
    
//     INode* inode = &inodes[foundInodeIndex];
//     inode->isUsed = 0;
//     printf("File '%s' removed successfully\n", path);
// }


void MakeDirectory(char *path) {
    INode* inodes = (INode*)(virtualDisk + sizeof(SuperBlock));
    int inodeNum = allocateInode();
    if (inodeNum == -1) {
        printf("No free inode available\n");
        return;
    }

    INode *inode = &inodes[inodeNum];
    strncpy(inode->fileName, path, sizeof(inode->fileName) - 1);
    inode->fileType = 1;  // 資料夾
    inode->isUsed = 1;
    inode->createTime = time(NULL);
    inode->modifyTime = time(NULL);
    printf("資料夾 '%s' 已建立！\n", path);
}

void RemoveDirectory(char *path){ // rmdir
    INode* inodes = (INode*)(virtualDisk + sizeof(SuperBlock));
    int foundInodeIndex = -1;
    
    for(int i = 0; i < sb->inodeCount; i++) {
        if(inodes[i].isUsed && strcmp(inodes[i].fileName, path) == 0) {
            foundInodeIndex = i;
            break;
        }
    }
    
    if(foundInodeIndex == -1) {
        printf("Directory '%s' not found\n", path);
        return;
    }
    
    INode* inode = &inodes[foundInodeIndex];
    if(inode->fileType != 1) {
        printf("'%s' is not a directory\n", path);
        return;
    }
    
    inode->isUsed = 0;
    printf("Directory '%s' removed successfully\n", path);
}

int PutFile(char *path) { 
    if(path == NULL) {
        printf("Invalid file name\n");
        return 1;
    }

    // 檢查虛擬磁碟是否已初始化
    if(virtualDisk == NULL || sb == NULL) {
        printf("File system not initialized\n");
        return 1;
    }

    // 1. 先檢查檔案是否已存在
    INode* inodes = (INode*)(virtualDisk + sizeof(SuperBlock));

    if(inodes == NULL) {
        printf("Failed to access inode table\n");
        return 1;
    }

    for(int i = 0; i < sb->inodeCount; i++) {
        if(inodes[i].isUsed && strcmp(inodes[i].fileName, path) == 0) {
            printf("File '%s' already exists\n", path);
            return 1;
        }
    }

    // 2. 分配 inode
    int inodeNum = allocateInode();
    if(inodeNum == -1) {
        printf("No free inode available\n");
        return 1;
    }

    // 3. 初始化 inode
    INode* inode = &inodes[inodeNum];
    strncpy(inode->fileName, path, sizeof(inode->fileName) - 1);
    inode->size = 0;
    inode->isUsed = 1;
    inode->createTime = time(NULL);
    inode->modifyTime = time(NULL);
    
    // 4. 初始化區塊指標
    for(int i = 0; i < 10; i++) {
        inode->directBlocks[i] = -1;
    }
    inode->indirectBlock = -1;

    printf("File '%s' created successfully\n", path);
    return 0;
}

int GetFile(char *path) {
   // 1. 找到檔案的 inode
   INode* inodes = (INode*)(virtualDisk + sizeof(SuperBlock));
   int foundInodeIndex = -1;
   
    for(int i = 0; i < sb->inodeCount; i++) {
        if(inodes[i].isUsed && strcmp(inodes[i].fileName, path) == 0) {
            foundInodeIndex = i;
            break;
        }
    }
   
    if(foundInodeIndex == -1) {
        printf("File '%s' not found\n", path);
        return 1;
    }

    INode* inode = &inodes[foundInodeIndex];
    
    // 2. 建立實體檔案
    FILE *fp = fopen(path, "w");
    if(fp == NULL) {
        printf("Cannot create file '%s'\n", path);
        return 1;
    }

    int remainSize = inode->size;  // 追蹤還需要寫入的檔案大小
    
    // 3. 處理直接區塊
    for(int i = 0; i < 10 && remainSize > 0; i++) {
        if(inode->directBlocks[i] == -1) break;
        
        char* blockStart = virtualDisk + sb->firstDataBlock * BLOCKSIZE + 
                            inode->directBlocks[i] * BLOCKSIZE;
        
        // 計算這個區塊要寫入多少內容
        int writeSize = (remainSize > BLOCKSIZE) ? BLOCKSIZE : remainSize;
        fwrite(blockStart, 1, writeSize, fp);
        remainSize -= writeSize;
    }

    // 4. 處理間接區塊
    if(remainSize > 0 && inode->indirectBlock != -1) {
        // 取得間接區塊表的位置
        int* indirectTable = (int*)(virtualDisk + 
                                    sb->firstDataBlock * BLOCKSIZE + 
                                    inode->indirectBlock * BLOCKSIZE);
        
        // 讀取間接區塊表中的每個區塊
        for(int i = 0; i < BLOCKSIZE/sizeof(int) && remainSize > 0; i++){
            if(indirectTable[i] == -1) break;
            
            char* blockStart = virtualDisk + 
                                sb->firstDataBlock * BLOCKSIZE + 
                                indirectTable[i] * BLOCKSIZE;
            
            int writeSize = (remainSize > BLOCKSIZE) ? BLOCKSIZE : remainSize;
            fwrite(blockStart, 1, writeSize, fp);
            remainSize -= writeSize;
        }
    }

   fclose(fp);
   printf("File '%s' retrieved successfully (size: %d bytes)\n", path, inode->size);
   return 0;
}

int ViEditor(char *path){ // vi
    FILE *fp = fopen(path, "w");
    if(fp == NULL){
        printf("file open error\n");
        return 1;
    }
    printf("> \n");
    char content[256];
    scanf("%s", content);
    fprintf(fp, "%s\n", content);
    fclose(fp);
    return 0;
}


// void DisplayFileContent(char *path){ //furthur

// }

void DisplayStatus(){
    printf("partition size: %d\n", sb->partitionSize);
    printf("total inodes: %d\n", sb->inodeCount);
    printf("used inodes: %d\n", sb->usedInodeCount);
    printf("total blocks: %d\n", sb->blockCount);
    printf("files's blocks: %d\n", sb->freeBlockCount);
    printf("block size: %d\n", sb->blockSize);
    printf("free space: %d\n", sb->freeBlockCount * sb->blockSize);
    printf("mount time: %s\n\n", ctime(&sb->mountTime));
}

void Help(void){
    printf("List of commands:\n");
    printf("'ls' - List all files in the current directory\n");
    printf("'cd' - Change directory\n");
    printf("'rm' - Remove file\n");
    printf("'mkdir' - Create directory\n");
    printf("'rmdir' - Remove directory\n");
    printf("'put' - Put file into the space\n");
    printf("'get' - Get file from the space\n");
    printf("'cat' - Display file content\n");
    printf("'status' - Display the status of the space\n");
    printf("'help' - Display the list of commands\n");
    printf("'exit and store image' - Exit the program and store the image\n");
}

// void ExitAndStoreImage(){
//     FILE *fp = fopen("vdisk", "w");
//     if(fp == NULL){
//         printf("file open error\n");
//         return;
//     }
//     fwrite(virtualDisk, sizeof(virtualDisk), 1, fp);
//     fclose(fp);
//     exit(0);
// }

void LoadDumpImage(void){
    FILE* fp;
    char LoadFile[20];

    scanf("%s", LoadFile);

    fp = fopen(LoadFile, "r+");
    if (fp == NULL) {
        printf("Failed to open the file.\n");
    }
    else {
        printf("Load Success.\n");
    }
}