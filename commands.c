#include "commands.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include "vi.h"

char currentPath[MAX_PATH_LEN] = "/";
INode* currentDir = NULL;
char fullPath[MAX_PATH_LEN];

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

// List all files in the current directory
void ListFiles(void) {
    INode* inodes = (INode*)(virtualDisk + sizeof(SuperBlock));
    printf("\nContents of directory %s:\n", currentPath);
    printf("-----------------------------------------------------------------------------\n");
    printf("Filename\tType\t\tSize\t\tCreation Time\n");
    printf("-----------------------------------------------------------------------------\n");

    for (int i = 0; i < sb->inodeCount; i++) {
        if (inodes[i].isUsed) {
            // Check if the file belongs to the current directory
            if (strcmp(currentPath, "/") == 0) {
                // For the root directory, show files directly under it
                char* firstSlash = strchr(inodes[i].fileName + 1, '/'); // +1 to skip the initial '/'
                if (!firstSlash) {
                    char timeStr[26];
                    ctime_s(timeStr, sizeof(timeStr), &inodes[i].createTime);
                    timeStr[24] = '\0';
                    printf("%-15s\t%s\t%4d bytes\t%s\n", 
                           inodes[i].fileName + 1, // Skip the initial '/'
                           inodes[i].fileType == 1 ? "Directory" : "File",
                           inodes[i].size,
                           timeStr);
                }
            } else {
                // For other directories, check full paths
                char* prefix = malloc(strlen(currentPath) + 2);
                sprintf(prefix, "%s/", currentPath + 1); // +1 to skip the initial '/'

                if (strncmp(inodes[i].fileName + 1, prefix, strlen(prefix)) == 0 &&
                    strchr(inodes[i].fileName + strlen(prefix) + 1, '/') == NULL) {
                    // Show file name without the full path
                    char* name = inodes[i].fileName + strlen(prefix);
                    if (name[0] == '/') {
                        name++;
                    }
                    char timeStr[26];
                    ctime_s(timeStr, sizeof(timeStr), &inodes[i].createTime);
                    timeStr[24] = '\0';
                    printf("%-15s\t%s\t%d bytes\t%s\n", 
                           name,
                           inodes[i].fileType == 1 ? "Directory" : "File",
                           inodes[i].size,
                           timeStr);
                }
                free(prefix);
            }
        }
    }
    printf("-----------------------------------------------------------------------------\n");
}

void ChangeDirectory(char *path) {
    INode* inodes = (INode*)(virtualDisk + sizeof(SuperBlock));
    int foundInodeIndex = -1;

    // Construct the full path for comparison
    char fullPath[256];
    if(strcmp(currentPath, "/") == 0) {
        snprintf(fullPath, sizeof(fullPath), "/%s", path);
    } else {
        snprintf(fullPath, sizeof(fullPath), "%s/%s", currentPath, path);
    }
    
    // Handle special case ".."
    if(strcmp(path, "..") == 0) {
        // If already in the root directory
        if(strcmp(currentPath, "/") == 0) {
            return;
        }
        
        // Go back to the parent directory
        char* lastSlash = strrchr(currentPath, '/');
        if(lastSlash != currentPath) {
            *lastSlash = '\0';
        } else {
            *(lastSlash + 1) = '\0';
        }
        printf("Current directory: %s\n", currentPath);
        return;
    }

    // Find the target directory
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
    
    // Update the current path
    strncpy(currentPath, fullPath, sizeof(currentPath) - 1);
    
    // Update the current directory
    currentDir = &inodes[foundInodeIndex];
    printf("Changed to directory: %s\n", currentPath);
}

void RemoveFile(char *path) {
    // 1. Check and find the inode of the file
    INode* inodes = (INode*)(virtualDisk + sizeof(SuperBlock));
    int foundInodeIndex = -1;
    
    if(strcmp(currentPath, "/") == 0){
        snprintf(fullPath, sizeof(fullPath), "%s%s", currentPath, path);
    } else {
        snprintf(fullPath, sizeof(fullPath), "%s/%s", currentPath, path);
    }

    for(int i = 0; i < sb->inodeCount; i++) {
        if(inodes[i].isUsed && strcmp(inodes[i].fileName, fullPath) == 0 && inodes[i].fileType == 0) {
            foundInodeIndex = i;
            break;
        }
    }
    
    if(foundInodeIndex == -1) {
        printf("File '%s' not found\n", path);
        return;
    }

    // 2. Release all direct blocks
    INode* inode = &inodes[foundInodeIndex];
    for(int i = 0; i < 10; i++) {
        if(inode->directBlocks[i] != -1) {
            freeBlock(inode->directBlocks[i]);
            inode->directBlocks[i] = -1;
        }
    }

    // 3. Handle indirect blocks (if any)
    if(inode->indirectBlock != -1) {
        // Get the indirect block table
        int* indirectTable = (int*)(virtualDisk + 
                                  sb->firstDataBlock * BLOCKSIZE + 
                                  inode->indirectBlock * BLOCKSIZE);
        
        // Release all pointed blocks
        for(int i = 0; i < BLOCKSIZE/sizeof(int); i++) {
            if(indirectTable[i] != -1) {
                freeBlock(indirectTable[i]);
            }
        }
        // Release the indirect block itself
        freeBlock(inode->indirectBlock);
        inode->indirectBlock = -1;
    }

    // 4. Update inode information
    inode->isUsed = 0;
    inode->size = 0;
    memset(inode->fileName, 0, sizeof(inode->fileName));
    
    // 5. Update system counters
    sb->freeInodeCount++;
    sb->usedInodeCount--;

    printf("File '%s' removed successfully\n", path);
}

void MakeDirectory(char *path) {
    // 1. Construct the full path
    char fullPath[256];
    if(strcmp(currentPath, "/") == 0) {
        snprintf(fullPath, sizeof(fullPath), "%s%s", currentPath, path);
    } else {
        snprintf(fullPath, sizeof(fullPath), "%s/%s", currentPath, path);
    }

    // 1. Check if the directory name already exists
    INode* inodes = (INode*)(virtualDisk + sizeof(SuperBlock));
    for(int i = 0; i < sb->inodeCount; i++) {
        if(inodes[i].isUsed && strcmp(inodes[i].fileName, path) == 0) {
            printf("Directory '%s' already exists\n", fullPath);
            return;
        }
    }

    // 2. Allocate an inode
    int inodeNum = allocateInode();
    if(inodeNum == -1) {
        printf("No free inode available\n");
        return;
    }

    // 3. Initialize the inode for the directory
    INode* inode = &inodes[inodeNum];
    strncpy(inode->fileName, path, sizeof(inode->fileName) - 1);
    inode->fileType = 1;  // 1 indicates directory
    inode->isUsed = 1;
    inode->size = 0;
    inode->createTime = time(NULL);
    inode->modifyTime = time(NULL);
    inode->permissions = 0755;  // Default permissions for directory

    // 4. Initialize the content of the directory
    for(int i = 0; i < 10; i++) {
        inode->directBlocks[i] = -1;
    }
    inode->indirectBlock = -1;
    
    // 5. Allocate the first block to store directory entries
    int blockNum = allocateBlock();
    if(blockNum == -1) {
        // If block allocation fails, rollback
        freeInode(inodeNum);
        printf("Failed to allocate block for directory\n");
        return;
    }
    inode->directBlocks[0] = blockNum;

    // 6. Update system counters
    sb->filesBlockCount++;

    // Use the full path when creating the directory
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
        if(inodes[i].isUsed && strcmp(inodes[i].fileName, fullPath) == 0 && inodes[i].fileType == 1) {
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
    // Check if the file already exists in the current directory
    INode* inodes = (INode*)(virtualDisk + sizeof(SuperBlock));
    char fullPath[MAX_PATH_LEN];
    
    // Construct the full path
    if (strcmp(currentPath, "/") == 0) {
        snprintf(fullPath, sizeof(fullPath), "%s%s", currentPath, path);
    } else {
        snprintf(fullPath, sizeof(fullPath), "%s/%s", currentPath, path);
    }

    // Check if the file already exists
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
    inode->fileType = 0;  // 0 indicates regular file
    inode->createTime = time(NULL);
    inode->modifyTime = time(NULL);
    
    for(int i = 0; i < 10; i++) {
        inode->directBlocks[i] = -1;
    }
    inode->indirectBlock = -1;

    printf("File '%s' created successfully\n", fullPath);
    return 0;
}


// Add a helper function to export a directory
int ExportDirectory(char *path, INode* dirInode){
   // 1. Create the target directory
    #ifdef _WIN32
        if(mkdir(path) != 0) {
    #else
        if(mkdir(path, 0755) != 0) {
    #endif
        printf("Failed to create directory '%s'\n", path);
        return 1;
    }

    // 2. Traverse all blocks of the directory
    INode* inodes = (INode*)(virtualDisk + sizeof(SuperBlock));
    
    for(int i = 0; i < 10; i++) {
        if(dirInode->directBlocks[i] == -1) {
            continue;
        }
        
        // Get the directory block
        DirEntry* entries = (DirEntry*)(virtualDisk + 
                                                    sb->firstDataBlock * BLOCKSIZE + 
                                                    dirInode->directBlocks[i] * BLOCKSIZE);
        
        // Traverse the directory entries in the block
        int entriesPerBlock = BLOCKSIZE / sizeof(DirEntry);
        for(int j = 0; j < entriesPerBlock; j++) {
            if(entries[j].inodeNumber == 0) {  // Empty entry
                continue;
            }
            
            INode* childInode = &inodes[entries[j].inodeNumber];
            char childPath[512];
            snprintf(childPath, sizeof(childPath), "%s/%s", path, entries[j].name);
            
            if(childInode->fileType == 1) {  // Directory
                if(ExportDirectory(childPath, childInode) != 0) {
                    return 1;
                }
            } else {  // File
                GetFile(childPath);
            }
        }
    }

    return 0;
}

int GetFile(char *path){
// 1. Check and create the virtualFileSystem directory
    struct stat st = {0};
    if (stat("virtualFileSystem", &st) == -1) {  // Check if the directory exists
        #ifdef _WIN32
            if(mkdir("virtualFileSystem") != 0) {
                printf("Failed to create virtualFileSystem directory\n");
                return 1;
            }
        #else
            if(mkdir("virtualFileSystem", 0755) != 0) {
                printf("Failed to create virtualFileSystem directory\n");
                return 1;
            }
        #endif
        printf("Created virtualFileSystem directory\n");
    }

    // 1. Find the inode of the file
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

    // 3. Create the output path
    char outputPath[512];
    snprintf(outputPath, sizeof(outputPath), "virtualFileSystem/%s", path);

    // Check if it is a directory
    if(inode->fileType == 1) {  // 1 indicates directory
        // Create the directory in the physical file system
        #ifdef _WIN32
            if(mkdir(outputPath) != 0) {
        #else
            if(mkdir(outputPath, 0755) != 0) {
        #endif
                printf("Failed to create directory '%s'\n", outputPath);
                return 1;
            }
        printf("Directory '%s' exported successfully\n", outputPath);
        return 0;
    }
    
    // 2. Create the physical file
    FILE *fp = fopen(outputPath, "w");
    if(fp == NULL) {
        printf("Cannot create file '%s'\n", outputPath);
        return 1;
    }

    int remainSize = inode->size;  // Track the remaining file size to be written
    
    // 3. Handle direct blocks
    for(int i = 0; i < 10 && remainSize > 0; i++) {
        if(inode->directBlocks[i] == -1) break;
        
        char* blockStart = virtualDisk + sb->firstDataBlock * BLOCKSIZE + 
                            inode->directBlocks[i] * BLOCKSIZE;
        
        // Calculate how much content to write for this block
        int writeSize = (remainSize > BLOCKSIZE) ? BLOCKSIZE : remainSize;
        fwrite(blockStart, 1, writeSize, fp);
        remainSize -= writeSize;
    }

    // 4. Handle indirect blocks
    if(remainSize > 0 && inode->indirectBlock != -1) {
        // Get the location of the indirect block table
        int* indirectTable = (int*)(virtualDisk + 
                                    sb->firstDataBlock * BLOCKSIZE + 
                                    inode->indirectBlock * BLOCKSIZE);
        
        // Read each block in the indirect block table
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
   printf("File '%s' retrieved successfully to virtualFileSystem/ (size: %d bytes)\n", 
           path, inode->size);
   return 0;
}

int ViEditor(char *path) {
    // Define pointer to inode and other necessary variables
    INode* inodes = (INode*)(virtualDisk + sizeof(SuperBlock));
    int foundInodeIndex = -1;

    // Construct the full path
    char fullPath[256];
    ConstructFullPath(fullPath, path);

    // Search for the file
    foundInodeIndex = SearchInode(fullPath, inodes);

    if (foundInodeIndex != -1) {
        // If file exists, read and display its content
        INode* inode = &inodes[foundInodeIndex];
        if (inode->fileType == 1) {
            printf("'%s' is a directory\n", path);
            return 1;
        }

        char* content = ReadFileContent(inode);
        if (!content) {
            fprintf(stderr, "Failed to read file content\n");
            return 1;
        }

        printf("File last modified: %s", ctime(&inode->modifyTime));

        // Enter vi editor mode
        ViEditorInteractive(inode, content);
        free(content);

    } else {
        // If file does not exist, create a new file
        printf("File '%s' not found. Creating a new file...\n", path);

        foundInodeIndex = allocateInode();
        if (foundInodeIndex == -1) {
            printf("No free inode available\n");
            return 1;
        }

        // Initialize the new inode
        INode* inode = &inodes[foundInodeIndex];
        InitializeNewInode(inode, fullPath);

        // Enter vi editor mode
        char emptyContent[1] = "\0";
        ViEditorInteractive(inode, emptyContent);
    }

    // Keep the terminal active after quitting the vi editor
    printf("Returning to command mode.\n");
    return 0;
}

void ConstructFullPath(char* fullPath, const char* path) {
    if (strcmp(currentPath, "/") == 0) {
        snprintf(fullPath, MAX_PATH_LEN - 1, "%s%s", currentPath, path);
    } else {
        snprintf(fullPath, MAX_PATH_LEN - 1, "%s/%s", currentPath, path);
    }
    fullPath[MAX_PATH_LEN - 1] = '\0'; // Ensure null termination
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

void ExitAndStoreImage(void) {
    if (!virtualDisk || !sb || sb->partitionSize <= 0) {
        printf("Error: Virtual disk or SuperBlock is not properly initialized.\n");
        return;
    }

    char password[32];
    printf("Set a password to protect the dump file: ");
    if (scanf("%31s", password) != 1 || strlen(password) == 0) {
        printf("Error: Password cannot be empty.\n");
        return;
    }
    int passwordLength = strlen(password);

    // 加密數據區
    printf("Encrypting the file system...\n");
    EncryptVirtualDisk(virtualDisk, sb->partitionSize, password, passwordLength);

    // 將加密後的虛擬磁碟保存到文件
    FILE* fp = fopen("disk_image.bin", "wb");
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



int LoadDumpImage(char* path) {
    FILE* fp = fopen(path, "rb");
    if (fp == NULL) {
        printf("Failed to open disk image file.\n\n");
        return 1;
    }

    // 1. 先讀取 SuperBlock 來取得大小資訊
    SuperBlock tempSb;
    if (fread(&tempSb, sizeof(SuperBlock), 1, fp) != 1) {
        printf("Failed to read SuperBlock\n");
        fclose(fp);
        return 1;
    }
    
    // 2. 回到檔案開頭
    fseek(fp, 0, SEEK_SET);

    // 3. 分配記憶體
    virtualDisk = (char*)malloc(tempSb.partitionSize);
    if (virtualDisk == NULL) {
        printf("Failed to allocate memory\n");
        fclose(fp);
        return 1;
    }

    // 4. 讀取整個映像
    size_t readSize = fread(virtualDisk, 1, tempSb.partitionSize, fp);
    if (readSize != tempSb.partitionSize) {
        printf("Warning: Only %zu bytes read (expected %d bytes).\n", readSize, tempSb.partitionSize);
    }

    fclose(fp);

    // 解密過程
    char password[32];
    printf("Enter the password to decrypt the file: ");
    if (scanf("%31s", password) != 1 || strlen(password) == 0) {
        printf("Error: Password cannot be empty.\n");
        free(virtualDisk);
        return 1;
    }
    int passwordLength = strlen(password);

    printf("Decrypting the file system...\n");
    DecryptVirtualDisk(virtualDisk, tempSb.partitionSize, password, passwordLength);

    // 5. 設置 SuperBlock 指標和當前路徑
    sb = (SuperBlock*)virtualDisk;

    // 初始化當前目錄指向根目錄
    strcpy(currentPath, "/");
    INode* inodes = (INode*)(virtualDisk + sizeof(SuperBlock));
    currentDir = &inodes[0];
    if (!currentDir->isUsed || currentDir->fileType != 1) {
        printf("Error: Root directory inode is invalid.\n");
        free(virtualDisk);
        return 1;
    }

    printf("Disk image loaded successfully (%zu bytes read).\n", readSize);
    return 0;
}


void EncryptVirtualDisk(char* virtualDisk, int partitionSize, char* password, int passwordLength) {
    SuperBlock* sb = (SuperBlock*)virtualDisk;
    int dataStart = sizeof(SuperBlock); // 從 SuperBlock 結束後開始
    int dataEnd = partitionSize;       // 到分區大小結束

    for (int i = dataStart; i < dataEnd; i++) {
        virtualDisk[i] ^= password[i % passwordLength];
    }
    printf("Encryption completed. Data range: %d to %d\n", dataStart, dataEnd);
}

void DecryptVirtualDisk(char* virtualDisk, int partitionSize, char* password, int passwordLength) {
    SuperBlock* sb = (SuperBlock*)virtualDisk;
    int dataStart = sizeof(SuperBlock); // 從 SuperBlock 結束後開始
    int dataEnd = partitionSize;       // 到分區大小結束

    for (int i = dataStart; i < dataEnd; i++) {
        virtualDisk[i] ^= password[i % passwordLength];
    }
    printf("Decryption completed. Data range: %d to %d\n", dataStart, dataEnd);
}
