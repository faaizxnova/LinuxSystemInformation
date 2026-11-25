#include <stdio.h>
#include <sys/statvfs.h>

int main(void) {
    struct statvfs info;

    if(statvfs("/", &info) == -1) {
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
    long double usage = (used_gb / total_gb) * 100;

    printf("Total : %.2Lf\n", total_gb);
    printf("Free : %.2Lf\n", free_gb);
    printf("Available : %.2Lf\n", available_gb);
    printf("Used : %.2Lf\n", used_gb);
    printf("Usage : %.2Lf", usage);

    return 0;
}