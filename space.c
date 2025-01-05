#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "space.h"

char* virtualDisk;
SuperBlock* sb;

int initFs(int size){
    // 1. 分配虛擬磁碟空間
    virtualDisk = (char*)malloc(size);
    if (virtualDisk == NULL) {
        return -1;  // 分配失敗
    }

    // 2. 初始化 superblock
    sb = (SuperBlock*)virtualDisk;
    sb->partitionSize = size;
    sb->blockSize = BLOCKSIZE;
    sb->inodeCount = MAXINODE;
    sb->blockCount = (size - sizeof(SuperBlock) - 
                     MAXINODE * sizeof(INode)) / BLOCKSIZE;
    sb->freeInodeCount = MAXINODE;
    sb->usedInodeCount = 0;
    sb->freeBlockCount = sb->blockCount;
    sb->usedBlockCount = 0;
    sb->filesBlockCount = 0;
    sb->firstDataBlock = (sizeof(SuperBlock) + 
                         MAXINODE * sizeof(INode) + 
                         sb->blockCount / 8 + BLOCKSIZE - 1) / BLOCKSIZE;
    sb->maxFileSize = BLOCKSIZE * (10 + BLOCKSIZE/sizeof(int));  // 直接塊+間接塊
    sb->magicNumber = 0x12345678;  // 自定義的魔數
    sb->mountTime = time(NULL);

    // 3. 初始化 inode 區域
    INode* inodes = (INode*)(virtualDisk + sizeof(SuperBlock));
    for(int i = 0; i < MAXINODE; i++) {
        inodes[i].isUsed = 0;
        inodes[i].size = 0;
        memset(inodes[i].fileName, 0, sizeof(inodes[i].fileName));
        for(int j = 0; j < 10; j++) {
            inodes[i].directBlocks[j] = -1;
        }
        inodes[i].indirectBlock = -1;
        inodes[i].createTime = time(NULL);
        inodes[i].modifyTime = time(NULL);
        inodes[i].fileType = 0;
        inodes[i].permissions = 0644;  // 預設權限
    }

    // 4. 初始化 block bitmap
    char* bitmap = virtualDisk + sizeof(SuperBlock) + MAXINODE * sizeof(INode);
    memset(bitmap, 0, sb->blockCount / 8);

    return 0;  // 初始化成功
}

// 尋找並分配空閒block
int getFreeBlock(void){
    if (sb->freeBlockCount == 0) {
        return -1;  // 沒有可用的block
    }

    char* bitmap = virtualDisk + sizeof(SuperBlock) + MAXINODE * sizeof(INode);
    
    // 搜尋第一個可用的block
    for (int i = 0; i < sb->blockCount; i++) {
        if ((bitmap[i / 8] & (1 << (i % 8))) == 0) {
            // 標記為已使用
            bitmap[i / 8] |= (1 << (i % 8));
            sb->freeBlockCount--;
            return i;
        }
    }
    
    return -1;
}

// 尋找並分配空閒inode
int getFreeInode(void){
    if (sb->freeInodeCount == 0) {
        return -1;
    }

    INode* inodes = (INode*)(virtualDisk + sizeof(SuperBlock));
    
    for (int i = 0; i < MAXINODE; i++) {
        if (!inodes[i].isUsed) {
            inodes[i].isUsed = 1;
            inodes[i].createTime = time(NULL);
            inodes[i].modifyTime = time(NULL);
            sb->freeInodeCount--;
            return i;
        }
    }
    
    return -1;
}

void freeInode(int inodeNum) {
   struct iNode* inodes = (struct iNode*)(virtualDisk + sizeof(struct superBlock));
   struct iNode* targetInode = &inodes[inodeNum];
   
   // 清空此inode所指向的blocks
   for(int i = 0; i < 10; i++) {
       if(targetInode->directBlocks[i] != -1) {
           freeBlock(targetInode->directBlocks[i]);
           targetInode->directBlocks[i] = -1;
       }
   }

   // 處理間接塊
    if(targetInode->indirectBlock != -1) {
        int* indirectBlockPtr = (int*)(virtualDisk + sb->firstDataBlock * BLOCKSIZE + 
                                     targetInode->indirectBlock * BLOCKSIZE);
        for(int i = 0; i < BLOCKSIZE/sizeof(int); i++) {
            if(indirectBlockPtr[i] != -1) {
                freeBlock(indirectBlockPtr[i]);
            }
        }
        freeBlock(targetInode->indirectBlock);
        targetInode->indirectBlock = -1;
    }

   // 重設inode狀態
   targetInode->size = 0;
   targetInode->isUsed = 0;
   memset(targetInode->fileName, 0, sizeof(targetInode->fileName));
   sb->freeInodeCount++;
   sb->usedInodeCount--;
}

void freeBlock(int blockNum) {
   char* bitmap = virtualDisk + sizeof(struct superBlock) + MAXINODE * sizeof(struct iNode);
   // 將對應bit設為0
   bitmap[blockNum / 8] &= ~(1 << (blockNum % 8));
   
   // 更新superblock
   sb->freeBlockCount++;
   sb->usedBlockCount--;
}

// 分配一個block並初始化它
int allocateBlock(void){
    int blockNum = getFreeBlock();
    if (blockNum == -1) {
        return -1;  // 分配失敗
    }

    // 計算block的實際位置並清空它
    char* blockStart = virtualDisk + sb->firstDataBlock * BLOCKSIZE + blockNum * BLOCKSIZE;
    memset(blockStart, 0, BLOCKSIZE);
    sb->usedBlockCount++;
    
    return blockNum;
}

// 分配一個inode並進行基本初始化
int allocateInode(void){
    int inodeNum = getFreeInode();
    if (inodeNum == -1) {
        return -1;  // 分配失敗
    }

    // 取得inode的指標
    INode* inodes = (INode*)(virtualDisk + sizeof(SuperBlock));
    INode* newInode = &inodes[inodeNum];

    // 初始化新的inode
    newInode->size = 0;
    newInode->isUsed = 1;
    newInode->createTime = time(NULL);
    newInode->modifyTime = time(NULL);
    newInode->fileType = 0;  // 假設0是一般檔案
    newInode->permissions = 0644;  // 基本權限
    newInode->indirectBlock = -1;
    
    // 初始化所有直接塊指標
    for (int i = 0; i < 10; i++) {
        newInode->directBlocks[i] = -1;
    }

    sb->usedInodeCount++;

    return inodeNum;
}



