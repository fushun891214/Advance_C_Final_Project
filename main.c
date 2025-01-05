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
        printf("  Input file name: ");
        LoadDumpImage();
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
        
        // char *ptr_Partition=calloc(*SizeOfPartition,sizeof(char));
        // if (ptr_Partition!=NULL){
        //     printf("Make new patition successful !\n");
        // }else{
        //     printf("Make new patition failed !\n");
        // }

        // Print the commands
        Help();

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
            
            // // else if(strcmp(cmd, "rmdir") == 0){
            //     char dirname[32];
            //     scanf("%s", dirname);
            //     ChangeDirectory(dirname); 
            //     ListFiles();
            //     RemoveFile();
            //     RemoveDirectory(dirname);
            // }
    
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

            else if(strcmp(cmd, "vi") == 0){
                char filename[32];
                scanf("%s", filename);
                printf("-----------------enter vi editor-----------------\n");
                ViEditor(filename);
                printf("-------------------------------------------------\n");
                printf("successfully saved file %s\n", filename);
            }
            
            else if (strcmp(cmd, "status") == 0){
                DisplayStatus();
            }
            else if(strcmp(cmd, "help") == 0){
                Help();
            }
            else if (strcmp(cmd, "exit") == 0){
                printf("Exit file system\n");
                // ExitAndStoreImage();
                break;
            }
            else{
                printf("Unknown command\n");
                Help();
            }
        }
    }
    else{
        printf("Invalid option\n");
        return -1;
    }
    return 0;
}