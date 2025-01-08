#ifndef SPACE_H
#define SPACE_H

#include <time.h>

#define BLOCKSIZE 1024
#define INODE_RATIO 16  // 表示 1/16 的空間給 inode

typedef struct superBlock {
   int partitionSize;       // Total space size
   int inodeCount;          // Total number of inodes
   int blockCount;          // Total number of blocks
   int freeInodeCount;      // Number of available inodes
   int usedInodeCount;      // Number of used inodes
   int freeBlockCount;      // Number of available blocks
   int usedBlockCount;      // Total number of used blocks
   int filesBlockCount;     // Number of blocks used by files
   int blockSize;           // Size of each block
   int firstDataBlock;      // Position of first data block
   time_t mountTime;        // Mount time
}SuperBlock;

// 定義目錄項結構
typedef struct dirEntry {
    char name[32];
    int inodeNumber;
}DirEntry;

typedef struct iNode {
    char fileName[32];          // File name
    int size;                   // File size (in bytes)
    int directBlocks[10];       // Directly pointed block locations
    int indirectBlock;          // Indirect block location
    int isUsed;                 // Whether it is in use
    time_t createTime;          // Creation time
    time_t modifyTime;          // Modification time
    int fileType;               // File type (regular/directory)
    int permissions;            // File permissions
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