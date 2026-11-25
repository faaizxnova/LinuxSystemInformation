#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h> 
#include <limits.h>
#include <errno.h>

int main(void)
{
    FILE * fp = fopen("/sys/block/nvme0n1/size", "r");

    if(fp == NULL) {
        perror("file is not open");
        return -1;
    }

    char line[256];
    char * end;
    short int erno = 0;

    fgets(line, sizeof(line), fp);

    unsigned long long size = strtoull(line, &end, 10);

    if(end == line) {
        perror("the string is in alphabets\n");
        return -1;
    }

    if(erno == ERANGE) {
        perror("Integer overflow detected\n");
        return -1;
    }

    printf("%llu", size);
    
    

    fclose(fp);

    return 0;
}
