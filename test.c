#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int main(void){
    // 製作檔案
    printf("file name:");
    char filename[256];
    scanf("%s", filename);
    FILE *fp = fopen(filename, "w"); // touch
    if(fp == NULL){
        printf("file open error\n");
        return 1;
    }
    printf("input:"); // put
    char input[256];
    scanf("%s", input);
    fprintf(fp, "%s\n", input);
    fclose(fp);
    return 0;

}