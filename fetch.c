#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <errno.h>
#include <limits.h>
#include <mntent.h>
#include <sys/statvfs.h>

typedef struct {
    double clock;
    char* v_id;
    char* mname;
    unsigned int logical_pross;
} cpu_info_t;

typedef struct {
    unsigned long long memTot;
    unsigned long long memFree;
    unsigned long long memAvail;
    unsigned long long buffer;
    unsigned long long cached;
    unsigned long long swapTot;
    unsigned long long swapFree;
} mem_info_t;

typedef struct {
    double totalGiB;
    double freeGiB;
    double availableGiB;
    double usedGiB;
    double usage;
    double capacity;

    unsigned long long secCount;

    unsigned int logical_block_size;
    unsigned int physical_block_size;

    char * model;

} storage_info_t;

int get_cpu_info(cpu_info_t * cpu) {
    FILE* fp;
    char line[256];
    char* info[4] = {"processor", "vendor_id", "model name", "cpu MHz"};
    int iter = 0;
    int i = 0;

    fp = fopen("/proc/cpuinfo", "r");

    if(fp == NULL) {
        perror("fopen\n");
        return -1;
    } 

    while(fgets(line, sizeof(line), fp) != NULL) {
        while(strncmp(line, info[i], strlen(info[i])) == 0) {
                char* parse = strchr(line, ':');
                char* end;
                parse ++;
                while(isspace((unsigned char)* parse)) parse ++;
                
                if(i == 1) {
                    cpu[iter].v_id = malloc(strlen(parse) + 1);
                    strncpy(cpu[iter].v_id, parse, strlen(parse));
                } if(i == 2) {
                    cpu[iter].mname = malloc(strlen(parse) + 1);
                    strncpy(cpu[iter].mname, parse, strlen(parse));
                } if(i == 3) {
                    cpu[iter].clock = strtod(parse, &end);
                    iter ++;
                    i = -1;
                } if(i == 0) {
                    cpu[iter].logical_pross = atoi(parse);
                } 
                
            i ++;
            
            if(iter == 11) return 0;
        }
    }

    return 0;
}

int get_mem_info(mem_info_t * mem) {
    FILE *fp;

    fp = fopen("/proc/meminfo", "r");

    if(fp == NULL) {
        perror("file is not open");
        return -1;
    }

    char line[256];
    char* memory[7] = {"MemTotal", "MemFree", "MemAvailable", "Buffers", "Cached", "SwapTotal", "SwapFree"};

    int i = 0;

    while(fgets(line, sizeof(line), fp) != NULL) {
        while(strncmp(line, memory[i], strlen(memory[i])) == 0) {
            char* parse = strchr(line, ':');
            char* end;

            parse ++;

            while(isspace((unsigned char) *parse)) parse ++;

            if(i == 0) mem -> memTot = strtoull(parse, &end, 10) / 1024;
            if(i == 1) mem -> memFree = strtoull(parse, &end, 10) / 1024; 
            if(i == 2) mem -> memAvail = strtoull(parse, &end, 10) / 1024; 
            if(i == 3) mem -> buffer = strtoull(parse, &end, 10) / 1024; 
            if(i == 4) mem -> cached = strtoull(parse, &end, 10) / 1024; 
            if(i == 5) mem -> swapTot = strtoull(parse, &end, 10) / 1024; 
            if(i == 6) mem -> swapFree = strtoull(parse, &end, 10) / 1024; 
            
            i ++;

            if(i == 7) return 0;
        }
    }

    return 0;
}

int read_sys_info(const char * path, unsigned int * value) {
    FILE * fp =  fopen(path, "r");

    if(fp == NULL) {
        perror("file is not opening\n");
        return -1;
    }

    char line[256];

    fgets(line, sizeof(line), fp);

    fclose(fp);

    char * start = line;
    char * end;

    while(isspace((unsigned int) *start)) start ++;

    if(*start == '-' || *start == '\0') {
        return -1;
    }

    errno = 0;

    unsigned long result = strtoul(start, &end, 10);

    if(*end == *start || errno == ERANGE || result > UINT_MAX) {
        return -1;
    }

    while(isspace((unsigned int) *end)) end ++;

    if(*end != '\0') {
        return -1;
    }

    *value = result;

    return 0;
}
int get_storage_info(storage_info_t *stg)
{
    FILE *fp1 = NULL;
    FILE *fp2 = NULL;
    FILE *fp3 = NULL;


    int result = -1;
    char line[256];
    char *end;

    const long double conversion =
        1024.0L * 1024.0L * 1024.0L;

    fp1 = setmntent("/proc/mounts", "r");
    fp2 = fopen("/sys/block/nvme0n1/device/model", "r");
    fp3 = fopen("/sys/block/nvme0n1/size", "r");

    if (fp1 == NULL || fp2 == NULL || fp3 == NULL) {
        perror("Opening storage interfaces");
        goto cleanup;
    }

    struct mntent *mnt;
    int found = 0;

    while ((mnt = getmntent(fp1)) != NULL) {

        if (strcmp(mnt->mnt_fsname, "/dev/nvme0n1p6") != 0)
            continue;

        struct statvfs info;

        if (statvfs(mnt->mnt_dir, &info) == -1) {
            perror("statvfs");
            goto cleanup;
        }

        if (info.f_frsize != 0 &&
            info.f_blocks > ULLONG_MAX / info.f_frsize) {
            fprintf(stderr, "Filesystem capacity overflow\n");
            goto cleanup;
        }

        unsigned long long total =
            (unsigned long long)info.f_blocks * info.f_frsize;

        unsigned long long free_space =
            (unsigned long long)info.f_bfree * info.f_frsize;

        unsigned long long available =
            (unsigned long long)info.f_bavail * info.f_frsize;

        if (free_space > total) {
            fprintf(stderr, "Invalid filesystem statistics\n");
            goto cleanup;
        }

        stg->totalGiB = (double)(total / conversion);
        stg->freeGiB = (double)(free_space / conversion);
        stg->availableGiB = (double)(available / conversion);
        stg->usedGiB = (double)((total - free_space) / conversion);

        stg->usage = total > 0
            ? ((double)(total - free_space) / (double)total) * 100.0
            : 0.0;

        found = 1;
        break;
    }

    if (!found) {
        fprintf(stderr, "Target filesystem not found\n");
        goto cleanup;
    }

    if (fgets(line, sizeof(line), fp2) == NULL) {
        fprintf(stderr, "Unable to read device model\n");
        goto cleanup;
    }

    line[strcspn(line, "\r\n")] = '\0';

    size_t len = strlen(line);

    while (len > 0 && isspace((unsigned char)line[len - 1]))
        line[--len] = '\0';

    stg->model = malloc(len + 1);

    if (stg->model == NULL) {
        perror("malloc");
        goto cleanup;
    }

    memcpy(stg->model, line, len + 1);

    if (fgets(line, sizeof(line), fp3) == NULL) {
        fprintf(stderr, "Unable to read sector count\n");
        goto cleanup;
    }

    char *start = line;

    while (isspace((unsigned char)*start))
        start++;

    if (*start == '-') {
        fprintf(stderr, "Negative sector count is invalid\n");
        goto cleanup;
    }

    errno = 0;

    stg->secCount = strtoull(start, &end, 10);

    if (start == end) {
        fprintf(stderr, "No valid sector count found\n");
        goto cleanup;
    }

    if (errno == ERANGE) {
        fprintf(stderr, "Sector count out of range\n");
        goto cleanup;
    }

    while (isspace((unsigned char)*end))
        end++;

    if (*end != '\0') {
        fprintf(stderr, "Unexpected characters in sector count\n");
        goto cleanup;
    }

    if (stg->secCount > ULLONG_MAX / 512ULL) {
        fprintf(stderr, "Device capacity overflow\n");
        goto cleanup;
    }

    unsigned long long bytesTotal = stg->secCount * 512ULL;

    stg->capacity = (double)((long double)bytesTotal / conversion);

    unsigned int value = 0;

    if(read_sys_info("/sys/block/nvme0n1/queue/physical_block_size", &value) == -1) {
        fprintf(stderr, "some error happen\n");
        return -1;
    }

    stg -> physical_block_size = value;

    if(read_sys_info("/sys/block/nvme0n1/queue/logical_block_size", &value) == -1) {
        fprintf(stderr, "some error happen\n");
        return -1;
    }

    stg -> logical_block_size = value;

    result = 0;

cleanup:

    if (fp1 != NULL)
        endmntent(fp1);

    if (fp2 != NULL)
        fclose(fp2);

    if (fp3 != NULL)
        fclose(fp3);

    if (result != 0) {
        free(stg->model);
        stg->model = NULL;
    }

    return result;
}

void displayInfoCPU(cpu_info_t * cpu) {
    for(int i = 0; i < 11; ++ i) {
        printf("\n%d\n", i + 1);
        printf("Processor : %d\n", cpu[i].logical_pross);
        printf("Vendor ID : %s", cpu[i].v_id);
        printf("Model name : %s", cpu[i].mname);
        printf("Clock : %.3f", cpu[i].clock);
        printf("\n==========================");
    }
}

void displayInfoMemory(mem_info_t * mem) {
    printf("Memory Total : %llu\n", mem -> memTot);
    printf("Memory Free : %llu\n", mem -> memFree);
    printf("Memory Available : %llu\n", mem -> memAvail);
    printf("Buffer : %llu\n", mem -> buffer);
    printf("Cached : %llu\n", mem -> cached);
    printf("Swap Total : %llu\n", mem -> swapTot);
    printf("Swap Total : %llu\n", mem -> swapFree);
    printf("\n==========================");
}

void displayInfoStorage(storage_info_t * nvme) {
    printf("Model : %s\n", nvme -> model);
    printf("Storage Total : %f\n", nvme -> totalGiB);
    printf("Storage Free : %f\n", nvme -> freeGiB);
    printf("Storage Available : %f\n", nvme -> availableGiB);
    printf("Used : %f\n", nvme -> usedGiB);
    printf("Usage : %f\n", nvme -> usage);
    printf("Capacity : %f\n", nvme -> capacity);
    printf("Sector Count : %llu\n", nvme -> secCount);
    printf("Physical Block : %u\n", nvme -> physical_block_size);
    printf("Logical Block : %u\n", nvme -> logical_block_size);
    printf("\n==========================");
}

int main(void) {
    cpu_info_t cpu[11];
    mem_info_t mem;
    storage_info_t nvme = {0};

    get_cpu_info(cpu);
    get_mem_info(&mem);
    get_storage_info(&nvme);

    //displayInfoCPU(cpu);
    // displayInfoMemory(&mem);
    displayInfoStorage(&nvme);
    
    return 0;
}
