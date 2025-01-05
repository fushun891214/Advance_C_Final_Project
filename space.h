#ifndef SPACE_H
#define SPACE_H

#include <time.h>

#define BLOCKSIZE 1024
#define INODE_RATIO 16  // 表示 1/16 的空間給 inode
// #define MAXBLOCK 2000
// #define MAXINODE 256

typedef struct superBlock {
   int partitionSize;       // 總空間大小
   int inodeCount;          // inode總數
   int blockCount;          // block總數
   int freeInodeCount;      // 可用inode數
   int usedInodeCount;      // 已使用的inode數
   int freeBlockCount;      // 可用block數
   int usedBlockCount;      // 已使用的block總數
   int filesBlockCount;     // 純檔案使用的block數
   int blockSize;           // 每個block大小
   int firstDataBlock;      // 第一個資料區塊位置
   int maxFileSize;         // 最大檔案大小限制
   int magicNumber;         // 檔案系統識別碼
   time_t mountTime;        // 掛載時間
}SuperBlock;

// 資料區塊：儲存實際的檔案內容
typedef struct block {
    char data[BLOCKSIZE];   // 實際儲存資料的空間
} Block;

typedef struct iNode {
    char fileName[32];          // 檔案名稱
    int size;                   // 檔案大小(bytes)
    int directBlocks[10];       // 直接指向的block位置
    int indirectBlock;          // 間接block位置
    int isUsed;                // 是否使用中
    time_t createTime;         // 建立時間
    time_t modifyTime;         // 修改時間
    int fileType;              // 檔案類型(一般/目錄)
    int permissions;           // 檔案權限
}INode;

int initFs(int size);
int allocateBlock(void);
int allocateInode(void);
int getFreeBlock(void);
void freeBlock(int blockNum);
int getFreeInode(void);
void freeInode(int inodeNum);

extern char* virtualDisk;
extern SuperBlock* sb;

#endif