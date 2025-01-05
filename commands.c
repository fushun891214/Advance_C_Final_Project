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

void HandleCommands(void){
    while(1){
        printf("%s $ ", currentPath);
        char cmd[256];
        scanf("%s", cmd);

        if(strcmp(cmd, "ls") == 0){
            ListFiles();
        }
        else if(strcmp(cmd, "cd") == 0){
            char dirname[32];
            scanf("%s", dirname);
            ChangeDirectory(dirname);
        }
        else if(strcmp(cmd, "rm") == 0){
            char filename[32];
            scanf("%s", filename);
            RemoveFile(filename);
        }
        else if(strcmp(cmd, "mkdir") == 0){
            char dirname[32];
            scanf("%s", dirname);
            MakeDirectory(dirname);
        }
        else if(strcmp(cmd, "rmdir") == 0){
            char dirname[32];
            scanf("%s", dirname);
            RemoveDirectory(dirname);
        }
        else if(strncmp(cmd, "put", 3) == 0){
            char filename[32];
            scanf("%s", filename);
            PutFile(filename);
        }
        else if(strncmp(cmd, "get", 3) == 0){
            char filename[32];
            scanf("%s", filename);
            GetFile(filename);
        }
        else if(strncmp(cmd, "cat", 3) == 0){
            char filename[32];
            scanf("%s", filename);
            DisplayFileContent(filename);
        }
        else if(strncmp(cmd, "vi", 2) == 0){
            char filename[32];
            scanf("%s", filename);
            ViEditor(filename);
        }
        else if (strcmp(cmd, "status") == 0){
            DisplayStatus();
        }
        else if(strcmp(cmd, "help") == 0){
            Help();
        }
        else if (strcmp(cmd, "exit") == 0){
            printf("Exit file system\n");
            ExitAndStoreImage();
            break;
        }
        else{
            printf("Unknown command\n");
            Help();
        }
    }
}

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

int ViEditor(char *path) {
    // 定義指向 inode 的指標和其他所需變數
    INode* inodes = (INode*)(virtualDisk + sizeof(SuperBlock));
    int foundInodeIndex = -1;

    // 構建完整路徑
    char fullPath[256];
    if (strcmp(currentPath, "/") == 0) {
        snprintf(fullPath, sizeof(fullPath), "%s%s", currentPath, path);
    } else {
        snprintf(fullPath, sizeof(fullPath), "%s/%s", currentPath, path);
    }

    // 搜尋檔案
    for (int i = 0; i < sb->inodeCount; i++) {
        if (inodes[i].isUsed && strcmp(inodes[i].fileName, fullPath) == 0) {
            foundInodeIndex = i;
            break;
        }
    }

    if (foundInodeIndex != -1) {
        // 如果檔案存在，讀取並顯示內容
        INode* inode = &inodes[foundInodeIndex];
        if (inode->fileType == 1) {
            printf("'%s' is a directory\n", path);
            return 1;
        }

        int remainSize = inode->size;
        char* content = (char*)malloc(inode->size + 1);
        if (!content) {
            fprintf(stderr, "Memory allocation failed\n");
            return 1;
        }

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
        printf("File content:\n%s\n", content);
        free(content);
    } else {
        // 如果檔案不存在，創建新檔案
        printf("File '%s' not found. Creating a new file...\n", path);

        foundInodeIndex = allocateInode();
        if (foundInodeIndex == -1) {
            printf("No free inode available\n");
            return 1;
        }

        // 獲取使用者輸入內容
        printf("-----------------enter vi editor-----------------\n");
        printf("Enter content (type ':q' on a new line to quit):\n");
        char content[1024];
        char line[256];
        content[0] = '\0';  // 初始化為空字串

        // 清空輸入緩衝區
        int c;
        while ((c = getchar()) != '\n' && c != EOF);
        
        while (1) {
            printf("> ");
            fgets(line, sizeof(line), stdin);
            if (strcmp(line, ":q\n") == 0){
                break;
            }
            strncat(content, line, sizeof(content) - strlen(content) - 1);
        }
        printf("-------------------------------------------------\n");

        // 將內容寫入檔案
        INode* inode = &inodes[foundInodeIndex];
        int contentLength = strlen(content);
        int numBlocks = (contentLength + BLOCKSIZE - 1) / BLOCKSIZE;

        // 分配直接區塊並寫入內容
        for (int i = 0; i < numBlocks && i < 10; i++) {
            int blockNum = allocateBlock();
            if (blockNum == -1) {
                printf("No free blocks available\n");
                return 1;
            }

            inode->directBlocks[i] = blockNum;
            char* blockStart = virtualDisk + sb->firstDataBlock * BLOCKSIZE + blockNum * BLOCKSIZE;

            int writeSize = ((i + 1) * BLOCKSIZE > contentLength) ? (contentLength - i * BLOCKSIZE) : BLOCKSIZE;
            memcpy(blockStart, content + i * BLOCKSIZE, writeSize);
        }

        // 更新 inode 資訊
        strncpy(inode->fileName, fullPath, sizeof(inode->fileName) - 1);
        inode->size = contentLength;
        inode->isUsed = 1;
        inode->fileType = 0;  // 一般檔案
        inode->modifyTime = time(NULL);
        inode->createTime = time(NULL);

        printf("File '%s' created successfully.\n", path);
    }

    return 0;
}



void DisplayFileContent(char *path){ //cat
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
    if(foundInodeIndex == -1){
        printf("File '%s' not found\n", path);
        return;
    }
    INode* inode = &inodes[foundInodeIndex];
    if(inode->fileType == 1){
        printf("'%s' is a directory\n", path);
        return;
    }
    int remainSize = inode->size;

    for (int i = 0; i < 10 && remainSize > 0; i++) {
        if (inode->directBlocks[i] == -1) break;

        char* blockStart = virtualDisk + sb->firstDataBlock * BLOCKSIZE + inode->directBlocks[i] * BLOCKSIZE;
        int readSize = (remainSize > BLOCKSIZE) ? BLOCKSIZE : remainSize;
        fwrite(blockStart, 1, readSize, stdout);
        remainSize -= readSize;
    }

    if (remainSize > 0 && inode->indirectBlock != -1) {
        int* indirectTable = (int*)(virtualDisk + sb->firstDataBlock * BLOCKSIZE + inode->indirectBlock * BLOCKSIZE);

        for (int i = 0; i < BLOCKSIZE / sizeof(int) && remainSize > 0; i++) {
            if (indirectTable[i] == -1) break;

            char* blockStart = virtualDisk + sb->firstDataBlock * BLOCKSIZE + indirectTable[i] * BLOCKSIZE;
            int readSize = (remainSize > BLOCKSIZE) ? BLOCKSIZE : remainSize;
            fwrite(blockStart, 1, readSize, stdout);
            remainSize -= readSize;
        }
    }

    printf("\n");


}

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

void ExitAndStoreImage(void){
    if (!virtualDisk || !sb) {
        printf("Error: virtualDisk or SuperBlock is not initialized.\n");
        return;
    }

    FILE *fp = fopen("disk_image.bin", "wb");
    if (fp == NULL) {
        printf("Failed to create disk image file.\n");
        return;
    }

    // 寫入 virtualDisk
    size_t writtenSize = fwrite(virtualDisk, 1, sb->partitionSize, fp);
    if (writtenSize != sb->partitionSize) {
        printf("Warning: Only %zu bytes written (expected %d bytes).\n", writtenSize, sb->partitionSize);
    }

    fclose(fp);
    printf("Disk image stored successfully (%zu bytes written).\n", writtenSize);
    exit(0);
}

int LoadDumpImage(char *path) {
    FILE *fp = fopen(path, "rb");
    if (fp == NULL) {
        printf("Failed to open disk image file.\n\n");
        return 1;
    }

    // 1. 先讀取 SuperBlock 來取得大小資訊
    SuperBlock tempSb;
    if(fread(&tempSb, sizeof(SuperBlock), 1, fp) != 1) {
        printf("Failed to read SuperBlock\n");
        fclose(fp);
        return 1;
    }
    
    // 2. 回到檔案開頭
    fseek(fp, 0, SEEK_SET);

    // 3. 分配記憶體
    virtualDisk = (char*)malloc(tempSb.partitionSize);
    if(virtualDisk == NULL) {
        printf("Failed to allocate memory\n");
        fclose(fp);
        return 1;
    }

    // 4. 讀取整個映像
    size_t readSize = fread(virtualDisk, 1, tempSb.partitionSize, fp);
    if(readSize != tempSb.partitionSize) {
        printf("Warning: Only %zu bytes read (expected %d bytes).\n", 
               readSize, tempSb.partitionSize);
    }

    // 5. 設置 SuperBlock 指標和當前路徑
    sb = (SuperBlock*)virtualDisk;
    strcpy(currentPath, "/");

    fclose(fp);
    printf("Disk image loaded successfully (%zu bytes read).\n", readSize);
    return 0;
}