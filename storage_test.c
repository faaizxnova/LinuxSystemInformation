#include <stdio.h>
#include <mntent.h>
#include <sys/statvfs.h>
#include <string.h>

int main(void)
{
    FILE * fp = setmntent("/proc/mounts", "r");

    if(fp == NULL) {
        perror("file is not open");
        return -1;
    }

    struct mntent * mnt;

    while((mnt = getmntent(fp))) {
        if((strcmp(mnt -> mnt_type, "proc") == 0 || strcmp(mnt -> mnt_type, "tmpfs") == 0 || strcmp(mnt -> mnt_type, "sysfs") == 0)) continue;
        
        printf("\n==================================\n");
        printf("Name : %s\n", mnt -> mnt_fsname);
        printf("Type : %s\n", mnt -> mnt_type);
        printf("Dir Name : %s\n", mnt -> mnt_dir);
            
        struct statvfs info;

        if(statvfs(mnt -> mnt_dir, &info) == -1) {
            perror("error occured");
            return -1;
        }

        unsigned long long total_bytes = (unsigned long long) info.f_blocks * info.f_frsize;
        unsigned long long free_bytes = (unsigned long long) info.f_bfree * info.f_frsize;
        unsigned long long available_bytes = (unsigned long long) info.f_bavail * info.f_frsize;
        unsigned long long used = total_bytes - free_bytes;

        long double conversion = 1024.0 * 1024.0 * 1024.0;

        long double total_gb = total_bytes / conversion; 
        long double free_gb = free_bytes / conversion; 
        long double available_gb = available_bytes / conversion;
        long double used_gb = used / conversion;
        long double usage;
        if(total_gb > 0) usage = (used_gb / total_gb) * 100;
        else usage = 0;

        printf("Total : %.2Lf\n", total_gb);
        printf("Free : %.2Lf\n", free_gb);
        printf("Available : %.2Lf\n", available_gb);
        printf("Used : %.2Lf\n", used_gb);
        printf("Usage : %.2Lf\n", usage);
    }

    if(endmntent(fp) == 1) printf("mnt succeed\n");
    else printf("process failed\n");

    return 0;
}
