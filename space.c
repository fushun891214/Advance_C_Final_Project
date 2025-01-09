#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "space.h"

char* virtualDisk;
SuperBlock* sb;

int initFs(int size) {
    // 1. Allocate and clear virtual disk space
    virtualDisk = (char*)malloc(size);
    if (virtualDisk == NULL) {
        printf("Failed to allocate memory\n");
        return -1;
    }
    memset(virtualDisk, 0, size);

    // 2. Initialize SuperBlock
    sb = (SuperBlock*)virtualDisk;
    sb->partitionSize = size;
    sb->blockSize = BLOCKSIZE;
    
    // 3. Calculate number of INodes
    sb->inodeCount = (size / (sizeof(INode) * INODE_RATIO));  // Use 1/16 space for inodes
    
    // 4. Initialize other fields
    sb->freeInodeCount = sb->inodeCount;
    sb->usedInodeCount = 0;
    sb->blockCount = (size - sizeof(SuperBlock) - 
                     sb->inodeCount * sizeof(INode)) / BLOCKSIZE;
    sb->freeBlockCount = sb->blockCount;
    sb->usedBlockCount = 0;
    sb->filesBlockCount = 0;
    sb->firstDataBlock = (sizeof(SuperBlock) + 
                        sb->inodeCount * sizeof(INode) + 
                        sb->blockCount / 8 + BLOCKSIZE - 1) / BLOCKSIZE;
    sb->mountTime = time(NULL);

    // 5. Initialize inode area
    INode* inodes = (INode*)(virtualDisk + sizeof(SuperBlock));
    for(int i = 0; i < sb->inodeCount; i++) {
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
        inodes[i].permissions = 0644;  // Default permissions

    }

    // 4. Initialize block bitmap
    char* bitmap = virtualDisk + sizeof(SuperBlock) + sb->inodeCount * sizeof(INode);
    memset(bitmap, 0, sb->blockCount / 8);

    return 0;
}

// Find and allocate a free block
int getFreeBlock(void){
    if (sb->freeBlockCount == 0) {
        return -1;  // No free blocks available
    }

    // Access the block bitmap in the virtual disk
    char* bitmap = virtualDisk + sizeof(SuperBlock) + sb->inodeCount * sizeof(INode);
    
    // Search for the first free block
    for (int i = 0; i < sb->blockCount; i++) {
        if ((bitmap[i / 8] & (1 << (i % 8))) == 0) { // Check if the block is free
            bitmap[i / 8] |= (1 << (i % 8)); // Mark the block as used
            sb->freeBlockCount--; // Decrease the free block count
            return i; // Return the index of the allocated block
        }
    }
    
    return -1; // Return -1 if no free blocks are found
}

// Find and allocate a free inode
int getFreeInode(void){
    // If there are no free inodes, return -1
    if (sb->freeInodeCount == 0) {
        return -1;
    }

    // Get the starting address of the inode table in the virtual disk
    INode* inodes = (INode*)(virtualDisk + sizeof(SuperBlock));
    
     // Iterate through all inodes to find a free one
    for (int i = 0; i < sb->inodeCount; i++) {
        // If the inode is not in use
        if (!inodes[i].isUsed) {
            inodes[i].isUsed = 1; // Mark the inode as used
            inodes[i].createTime = time(NULL); // Set the creation time
            inodes[i].modifyTime = time(NULL); // Set the modification time
            sb->freeInodeCount--; // Decrease the count of free inodes
            return i; // Return the index of the allocated inode
        }
    }

    // If no free inode is found, return -1
    return -1;
}

void freeInode(int inodeNum) {
   struct iNode* inodes = (struct iNode*)(virtualDisk + sizeof(struct superBlock)); // Access the inode table
   struct iNode* targetInode = &inodes[inodeNum]; // Get the specific inode to be freed
   
   // Clear all blocks pointed by this inode
   for(int i = 0; i < 10; i++) {
       if(targetInode->directBlocks[i] != -1) { // If the direct block is allocated
           freeBlock(targetInode->directBlocks[i]); // Free the block
           targetInode->directBlocks[i] = -1; // Mark the block as unallocated
       }
   }

   // Handle indirect block
    if(targetInode->indirectBlock != -1) { // If an indirect block exists
        int* indirectBlockPtr = (int*)(virtualDisk + sb->firstDataBlock * BLOCKSIZE + 
                                     targetInode->indirectBlock * BLOCKSIZE); // Access the indirect block
        for(int i = 0; i < BLOCKSIZE/sizeof(int); i++) { // Iterate through all entries in the indirect block
            if(indirectBlockPtr[i] != -1) { // If the entry points to a block
                freeBlock(indirectBlockPtr[i]); // Free the block
            }
        }
        freeBlock(targetInode->indirectBlock); // Free the indirect block itself
        targetInode->indirectBlock = -1; // Mark the indirect block as unallocated
    }

   // Reset the inode's state
   targetInode->size = 0; // Reset the file size
   targetInode->isUsed = 0; // Mark the inode as unused
   memset(targetInode->fileName, 0, sizeof(targetInode->fileName)); // Clear the file name
   sb->freeInodeCount++; // Increment the free inode count
   sb->usedInodeCount--; // Decrement the used inode count
}

void freeBlock(int blockNum) {
   // Access the block bitmap in the virtual disk
   char* bitmap = virtualDisk + sizeof(struct superBlock) + sb->inodeCount * sizeof(struct iNode);
   // Clear the corresponding bit to mark the block as free
   bitmap[blockNum / 8] &= ~(1 << (blockNum % 8));
   
   // Update the superblock to reflect the change
   sb->freeBlockCount++; // Increment the free block count
   sb->usedBlockCount--; // Decrement the used block count
}

// Allocate a block and initialize it
int allocateBlock(void){
    int blockNum = getFreeBlock(); // Get a free block from the bitmap
    if (blockNum == -1) {
        return -1;  // Allocation failed
    }

    // Calculate the block's actual location and clear its contents
    char* blockStart = virtualDisk + sb->firstDataBlock * BLOCKSIZE + blockNum * BLOCKSIZE;
    memset(blockStart, 0, BLOCKSIZE); // Initialize the block with zeros
    sb->usedBlockCount++; // Increment the used block count in the superblock
    
    return blockNum; // Return the allocated block number
}

// Allocate an inode and perform basic initialization
int allocateInode(void){
    int inodeNum = getFreeInode(); // Get a free inode from the inode list
    if (inodeNum == -1) {
        return -1;  // Allocation failed
    }

    // Retrieve the inode pointer
    INode* inodes = (INode*)(virtualDisk + sizeof(SuperBlock));
    INode* newInode = &inodes[inodeNum];

    // Initialize the new inode
    newInode->size = 0; // Set file size to 0
    newInode->isUsed = 1; // Mark inode as used
    newInode->createTime = time(NULL); // Set creation time
    newInode->modifyTime = time(NULL); // Set modification time
    newInode->fileType = 0;  // Assuming 0 is for regular files
    newInode->permissions = 0644;  // Set default permissions
    newInode->indirectBlock = -1; // No indirect block
    
    // Initialize all direct block pointers to -1 (indicating no blocks)
    for (int i = 0; i < 10; i++) {
        newInode->directBlocks[i] = -1;
    }

    sb->usedInodeCount++; // Increment the used inode count

    return inodeNum; // Return the index of the allocated inode
}