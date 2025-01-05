#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "space.h"
#include "commands.h"

int main(void){
    int option=0;
    printf("options:\n");
    printf(" 1. Load from file\n");
    printf(" 2. Create new partition in memory\n");
    scanf("%d", &option);
    if(option==1){
        printf("  Input file name: (example disk_image.bin)\n");
        char filename[32];
        while(1){
            scanf("%s", filename);
            if(LoadDumpImage(filename)!=0){
                Help();
                printf("\n\n  Input file name: (example disk_image.bin)\n");
            }
            else{
                printf("Successfully loaded file %s\n", filename);
                HandleCommands();
                break;
            }
        }
    }
    else if(option==2){    
        printf("  Input size of a new partition: (example 102400 204800)\n");
        int size = 0;
        scanf("%d",&size);
        printf("Partition size = %d\n",size);

        if (initFs(size) != 0) {
            printf("Failed to initialize file system\n");
            return -1;
        }
        
        Help();
        HandleCommands();
    }
    else{
        printf("Invalid option\n");
        return -1;
    }
    return 0;
}