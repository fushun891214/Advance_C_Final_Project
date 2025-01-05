#include "commands.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include "space.h"

char currentPath[MAX_PATH_LEN] = "/";
INode* currentDir = NULL;
char fullPath[256];

// 列出所有檔案
void ListFiles(void){
    INode* inodes = (INode*)(virtualDisk + sizeof(SuperBlock));
    printf("\n目錄 %s 的內容:\n", currentPath);
    printf("---------------------------------------------------------\n");
    printf("檔名\t\t類型\t\t大小\t\t建立時間\n");
    printf("---------------------------------------------------------\n");

    for (int i = 0; i < sb->inodeCount; i++) {
        if (inodes[i].isUsed) {
            // 檢查檔案是否屬於當前目錄
            if (strcmp(currentPath, "/") == 0) {
                // 根目錄的情況：只顯示沒有 '/' 的檔案或直接在根目錄下的檔案
                char* firstSlash = strchr(inodes[i].fileName + 1, '/');  // +1 跳過開頭的 '/'
                if (!firstSlash) {
                    char timeStr[26];
                    ctime_r(&inodes[i].createTime, timeStr);
                    timeStr[24] = '\0';
                    printf("%-15s\t%s\t%d bytes\t%s\n", 
                           inodes[i].fileName + 1,  // 跳過開頭的 '/'
                           inodes[i].fileType == 1 ? "目錄" : "檔案",
                           inodes[i].size,
                           timeStr);
                }
            } else {
                // 其他目錄的情況：檢查完整路徑
                char* prefix = malloc(strlen(currentPath) + 2);
                sprintf(prefix, "%s/", currentPath + 1);  // +1 跳過開頭的 '/'
                
                if (strncmp(inodes[i].fileName + 1, prefix, strlen(prefix)) == 0 &&
                    strchr(inodes[i].fileName + strlen(prefix) + 1, '/') == NULL) {
                    // 顯示不含路徑的檔案名
                    char* name = inodes[i].fileName + strlen(prefix);
                    if(name[0] == '/'){
                        name++;
                    }
                    char timeStr[26];
                    ctime_r(&inodes[i].createTime, timeStr);
                    timeStr[24] = '\0';
                    printf("%-15s\t%s\t%d bytes\t%s\n", 
                           name,
                           inodes[i].fileType == 1 ? "目錄" : "檔案",
                           inodes[i].size,
                           timeStr);
                }
                free(prefix);
            }
        }
    }
    printf("---------------------------------------------------------\n");
}

void ChangeDirectory(char *path) {
    INode* inodes = (INode*)(virtualDisk + sizeof(SuperBlock));
    int foundInodeIndex = -1;

    // 構建完整路徑以進行比對
    char fullPath[256];
    if(strcmp(currentPath, "/") == 0) {
        snprintf(fullPath, sizeof(fullPath), "/%s", path);
    } else {
        snprintf(fullPath, sizeof(fullPath), "%s/%s", currentPath, path);
    }
    
    // 處理特殊情況 ".."
    if(strcmp(path, "..") == 0) {
        // 如果已經在根目錄
        if(strcmp(currentPath, "/") == 0) {
            return;
        }
        
        // 回到上一層目錄
        char* lastSlash = strrchr(currentPath, '/');
        if(lastSlash != currentPath) {
            *lastSlash = '\0';
        } else {
            *(lastSlash + 1) = '\0';
        }
        printf("Current directory: %s\n", currentPath);
        return;
    }

    // // 尋找目標目錄
    for(int i = 0; i < sb->inodeCount; i++) {
        if(inodes[i].isUsed && 
           strcmp(inodes[i].fileName, fullPath) == 0 && 
           inodes[i].fileType == 1) {
            foundInodeIndex = i;
            break;
        }
    }
    
    if(foundInodeIndex == -1) {
        printf("Directory '%s' not found\n", path);
        return;
    }
    
    // 更新當前路徑
    strncpy(currentPath, fullPath, sizeof(currentPath) - 1);
    
    // 更新當前目錄
    currentDir = &inodes[foundInodeIndex];
    printf("Changed to directory: %s\n", currentPath);
}

void RemoveFile(char *path) {
    // 1. 檢查並找到檔案的 inode
    INode* inodes = (INode*)(virtualDisk + sizeof(SuperBlock));
    int foundInodeIndex = -1;
    
    if(strcmp(currentPath, "/") == 0){
        snprintf(fullPath, sizeof(fullPath), "%s%s", currentPath, path);
    } else {
        snprintf(fullPath, sizeof(fullPath), "%s/%s", currentPath, path);
    }

    for(int i = 0; i < sb->inodeCount; i++) {
        if(inodes[i].isUsed && strcmp(inodes[i].fileName, fullPath) == 0) {
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

void MakeDirectory(char *path) {
    // 1. 建構完整路徑
    char fullPath[256];
    if(strcmp(currentPath, "/") == 0) {
        snprintf(fullPath, sizeof(fullPath), "%s%s", currentPath, path);
    } else {
        snprintf(fullPath, sizeof(fullPath), "%s/%s", currentPath, path);
    }

    // 1. 檢查目錄名稱是否已存在
    INode* inodes = (INode*)(virtualDisk + sizeof(SuperBlock));
    for(int i = 0; i < sb->inodeCount; i++) {
        if(inodes[i].isUsed && strcmp(inodes[i].fileName, path) == 0) {
            printf("Directory '%s' already exists\n", fullPath);
            return;
        }
    }

    // 2. 分配 inode
    int inodeNum = allocateInode();
    if(inodeNum == -1) {
        printf("No free inode available\n");
        return;
    }

    // 3. 初始化目錄的 inode
    INode* inode = &inodes[inodeNum];
    strncpy(inode->fileName, path, sizeof(inode->fileName) - 1);
    inode->fileType = 1;  // 1 表示目錄
    inode->isUsed = 1;
    inode->size = 0;
    inode->createTime = time(NULL);
    inode->modifyTime = time(NULL);
    inode->permissions = 0755;  // 目錄的默認權限

    // 4. 初始化目錄的內容
    for(int i = 0; i < 10; i++) {
        inode->directBlocks[i] = -1;
    }
    inode->indirectBlock = -1;
    
    // 5. 分配第一個區塊來存儲目錄項
    int blockNum = allocateBlock();
    if(blockNum == -1) {
        // 如果分配區塊失敗，需要回滾
        freeInode(inodeNum);
        printf("Failed to allocate block for directory\n");
        return;
    }
    inode->directBlocks[0] = blockNum;

    // 6. 更新系統計數
    sb->filesBlockCount++;

    // 在創建目錄時使用完整路徑
    strncpy(inode->fileName, fullPath, sizeof(inode->fileName) - 1);
    printf("Directory '%s' created successfully\n", path);
}

void RemoveDirectory(char *path){ // rmdir
    INode* inodes = (INode*)(virtualDisk + sizeof(SuperBlock));
    int foundInodeIndex = -1;
    
    char fullPath[256];
    if(strcmp(currentPath, "/") == 0){
        snprintf(fullPath, sizeof(fullPath), "%s%s", currentPath, path);
    } else {
        snprintf(fullPath, sizeof(fullPath), "%s/%s", currentPath, path);
    }

    for(int i = 0; i < sb->inodeCount; i++) {
        if(inodes[i].isUsed && strcmp(inodes[i].fileName, fullPath) == 0) {
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
    // 檢查檔案是否已存在於當前目錄
    INode* inodes = (INode*)(virtualDisk + sizeof(SuperBlock));
    char fullPath[MAX_PATH_LEN];
    
    // 構建完整路徑
    if (strcmp(currentPath, "/") == 0) {
        snprintf(fullPath, sizeof(fullPath), "%s%s", currentPath, path);
    } else {
        snprintf(fullPath, sizeof(fullPath), "%s/%s", currentPath, path);
    }

    // 檢查檔案是否已存在
    for(int i = 0; i < sb->inodeCount; i++) {
        if(inodes[i].isUsed && strcmp(inodes[i].fileName, fullPath) == 0) {
            printf("File already exists in current directory\n");
            return 1;
        }
    }

    int inodeNum = allocateInode();
    if(inodeNum == -1) {
        printf("No free inode available\n");
        return 1;
    }

    INode* inode = &inodes[inodeNum];
    strncpy(inode->fileName, fullPath, sizeof(inode->fileName) - 1);
    inode->size = 0;
    inode->isUsed = 1;
    inode->fileType = 0;  // 0 表示一般檔案
    inode->createTime = time(NULL);
    inode->modifyTime = time(NULL);
    
    for(int i = 0; i < 10; i++) {
        inode->directBlocks[i] = -1;
    }
    inode->indirectBlock = -1;

    printf("File '%s' created successfully\n", fullPath);
    return 0;
}

// 新增一個輔助函數來導出目錄
int ExportDirectory(char *path, INode* dirInode){
    // 建立實體目錄
    #ifdef _WIN32
        if(mkdir(path) != 0) {
    #else
        if(mkdir(path, 0755) != 0) {
    #endif
        printf("Failed to create directory '%s'\n", path);
        return 1;
    }

    // 遍歷目錄的所有區塊，尋找子檔案和子目錄
    for(int i = 0; i < 10; i++) {
        if(dirInode->directBlocks[i] == -1) continue;
        
        // 取得目錄區塊
        Block* block = (Block*)(virtualDisk + sb->firstDataBlock * BLOCKSIZE + 
                              dirInode->directBlocks[i] * BLOCKSIZE);
        
        // 遍歷目錄中的每個項目
        INode* inodes = (INode*)(virtualDisk + sizeof(SuperBlock));
        for(int j = 0; j < sb->inodeCount; j++) {
            if(inodes[j].isUsed) {
                char fullPath[256];
                snprintf(fullPath, sizeof(fullPath), "%s/%s", path, inodes[j].fileName);
                
                if(inodes[j].fileType == 1) {  // 如果是目錄
                    ExportDirectory(fullPath, &inodes[j]);
                } else {  // 如果是檔案
                    GetFile(fullPath);
                }
            }
        }
    }

    return 0;
}

int GetFile(char *path){
    // 1. 找到檔案的 inode
    INode* inodes = (INode*)(virtualDisk + sizeof(SuperBlock));
    int foundInodeIndex = -1;
    char fullPath[256];
    if(strcmp(currentPath, "/") == 0){
        snprintf(fullPath, sizeof(fullPath), "%s%s", currentPath, path);
    } 
    else{
        snprintf(fullPath, sizeof(fullPath), "%s/%s", currentPath, path);
    }
    for(int i = 0; i < sb->inodeCount; i++){
        if(inodes[i].isUsed && strcmp(inodes[i].fileName, fullPath) == 0){ 
            foundInodeIndex = i;
            break;
        }
    }
   
    if(foundInodeIndex == -1) {
        printf("File '%s' not found\n", path);
        return 1;
    }

    INode* inode = &inodes[foundInodeIndex];

    // 檢查是否為目錄
    if(inode->fileType == 1) {  // 1 表示目錄
        // 在實體檔案系統建立目錄
        #ifdef _WIN32
            if(mkdir(path) != 0) {
        #else
            if(mkdir(path, 0755) != 0) {
        #endif
                printf("Failed to create directory '%s'\n", path);
                return 1;
            }
        printf("Directory '%s' exported successfully\n", path);
        return 0;
    }
    
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