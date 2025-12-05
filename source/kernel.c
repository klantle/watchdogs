#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "utils.h"
#include "kernel.h"

#ifdef WG_WINDOWS
    #include <windows.h>
    #include <iphlpapi.h>
    #include <intrin.h>
int
uname(struct utsname *name)
{
        OSVERSIONINFOEX osvi;
        SYSTEM_INFO si;

        ZeroMemory(&osvi, sizeof(OSVERSIONINFOEX));
        osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);

        if (!GetVersionEx((OSVERSIONINFO*)&osvi)) {
                return -1;
        }

        GetSystemInfo(&si);

        snprintf(name->sysname, sizeof(name->sysname), "Windows");
        snprintf(name->release, sizeof(name->release), "%lu.%lu",
                                osvi.dwMajorVersion, osvi.dwMinorVersion);
        snprintf(name->version, sizeof(name->version), "Build %lu",
                                osvi.dwBuildNumber);

        switch (si.wProcessorArchitecture) {
                case PROCESSOR_ARCHITECTURE_AMD64:
                        snprintf(name->machine, sizeof(name->machine), "x86_64");
                        break;
                case PROCESSOR_ARCHITECTURE_INTEL:
                        snprintf(name->machine, sizeof(name->machine), "x86");
                        break;
                case PROCESSOR_ARCHITECTURE_ARM:
                        snprintf(name->machine, sizeof(name->machine), "ARM");
                        break;
                case PROCESSOR_ARCHITECTURE_ARM64:
                        snprintf(name->machine, sizeof(name->machine), "ARM64");
                        break;
                default:
                        snprintf(name->machine, sizeof(name->machine), "Unknown");
                        break;
        }

        return 0;
}
#else
    #include <sys/statvfs.h>
    #include <sys/utsname.h>
    #include <ifaddrs.h>
    #include <net/if.h>
    #include <unistd.h>
#endif


/* ============================================================================
 * Field Display Utilities
 * ==========================================================================*/

void
hardware_display_field(unsigned int field_id, const char* format, ...)
{
        va_list args;
        va_start(args, format);

        const char* field_name = "";
        char prefix[32] = "";

        /* Determine field category and name */
        switch (field_id & 0xFF00) {
                case 0x0000: /* CPU fields */
                        strcpy(prefix, "CPU->");
                        switch (field_id) {
                                case FIELD_CPU_NAME:        field_name = "Processor"; break;
                                case FIELD_CPU_VENDOR:  field_name = "Vendor"; break;
                                case FIELD_CPU_CORES:   field_name = "Cores"; break;
                                case FIELD_CPU_THREADS: field_name = "Threads"; break;
                                case FIELD_CPU_CLOCK:   field_name = "Clock Speed"; break;
                                case FIELD_CPU_CACHE:   field_name = "Cache"; break;
                                case FIELD_CPU_ARCH:        field_name = "Architecture"; break;
                        }
                        break;

                case 0x0100: /* Memory fields */
                        strcpy(prefix, "MEM->");
                        switch (field_id) {
                                case FIELD_MEM_TOTAL: field_name = "Total RAM"; break;
                                case FIELD_MEM_AVAIL: field_name = "Available RAM"; break;
                                case FIELD_MEM_SPEED: field_name = "Speed"; break;
                                case FIELD_MEM_TYPE:  field_name = "Type"; break;
                        }
                        break;

                case 0x0200: /* Disk fields */
                        strcpy(prefix, "DISK->");
                        switch (field_id) {
                                case FIELD_DISK_MOUNT: field_name = "Mount Point"; break;
                                case FIELD_DISK_TOTAL: field_name = "Total Space"; break;
                                case FIELD_DISK_FREE:  field_name = "Free Space"; break;
                                case FIELD_DISK_USED:  field_name = "Used Space"; break;
                                case FIELD_DISK_FS:        field_name = "File System"; break;
                        }
                        break;

                default:
                        field_name = "Unknown";
                        break;
        }

        printf("%-8s%-20s: ", prefix, field_name);
        vprintf(format, args);
        printf("\n");

        va_end(args);
}


/* ============================================================================
 * Platform-Specific Implementations
 * ==========================================================================*/

#ifdef WG_WINDOWS

int
hardware_cpu_info(HardwareCPU* cpu)
{
        if (!cpu) {
                return 0;
        }

        memset(cpu, 0, sizeof(HardwareCPU));

        /* Get CPU brand */
        int cpuInfo[4] = { 0 };
        char brand[64] = { 0 };
        hardware_cpuid(cpuInfo, cpuid_cpuinfo);
        unsigned int maxId = cpuInfo[0];

        if (maxId >= cpu_maxid) {
                hardware_cpuid((int*)(brand), cpuid_brand);
                hardware_cpuid((int*)(brand+16), cpuid_brand_16);
                hardware_cpuid((int*)(brand+32), cpuid_brand_32);
                strncpy(cpu->name, brand, sizeof(cpu->name)-1);
        }

        /* Get vendor */
        hardware_cpuid(cpuInfo, 0);
        int vendor[4] = {cpuInfo[1], cpuInfo[3], cpuInfo[2], 0};
        strncpy(cpu->vendor, (char*)vendor, sizeof(cpu->vendor)-1);

        /* Get core info */
        SYSTEM_INFO si;
        GetSystemInfo(&si);
        cpu->cores = si.dwNumberOfProcessors;
        cpu->threads = si.dwNumberOfProcessors; /* Simplified */

        /* Get architecture */
        if (si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64) {
                strcpy(cpu->architecture, "x86_64");
        }
        else if (si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_INTEL) {
                strcpy(cpu->architecture, "x86");
        }
        else {
                strcpy(cpu->architecture, "Unknown");
        }

        return 1;
}

int
hardware_memory_info(HardwareMemory* mem)
{
        if (!mem) {
                return 0;
        }

        memset(mem, 0, sizeof(HardwareMemory));

        MEMORYSTATUSEX memInfo;
        memInfo.dwLength = sizeof(memInfo);
        
        if (GlobalMemoryStatusEx(&memInfo)) {
                mem->total_phys = memInfo.ullTotalPhys;
                mem->avail_phys = memInfo.ullAvailPhys;
                mem->total_virt = memInfo.ullTotalVirtual;
                mem->avail_virt = memInfo.ullAvailVirtual;
                return 1;
        }

        return 0;
}

int
hardware_disk_info(HardwareDisk* disk, const char* drive)
{
        if (!disk || !drive) {
                return 0;
        }

        memset(disk, 0, sizeof(HardwareDisk));
        strncpy(disk->mount_point, drive, sizeof(disk->mount_point)-1);

        ULARGE_INTEGER freeBytes, totalBytes, totalFree;
        if (GetDiskFreeSpaceEx(drive, &freeBytes, &totalBytes, &totalFree)) {
                disk->total_bytes = totalBytes.QuadPart;
                disk->free_bytes = freeBytes.QuadPart;
                disk->used_bytes = totalBytes.QuadPart - freeBytes.QuadPart;

                disk->total_gb = totalBytes.QuadPart / (1024.0 * 1024 * 1024);
                disk->free_gb = freeBytes.QuadPart / (1024.0 * 1024 * 1024);
                disk->used_gb = disk->used_bytes / (1024.0 * 1024 * 1024);

                /* Get filesystem type */
                char fsName[32];
                DWORD maxCompLen, fsFlags;
                if (GetVolumeInformation(drive, NULL, 0, NULL, &maxCompLen, 
                                                                &fsFlags, fsName, sizeof(fsName))) {
                        strncpy(disk->filesystem, fsName, sizeof(disk->filesystem)-1);
                }
                else {
                        strcpy(disk->filesystem, "Unknown");
                }

                return 1;
        }

        return 0;
}

#else /* UNIX/Linux implementation */

int
hardware_cpu_info(HardwareCPU* cpu)
{
        if (!cpu) {
                return 0;
        }

        memset(cpu, 0, sizeof(HardwareCPU));

        FILE *f = fopen("/proc/cpuinfo", "r");
        if (!f) {
                return 0;
        }

        char cpu_line[WG_PATH_MAX];

        while (fgets(cpu_line, sizeof(cpu_line), f)) {
                if (strncmp(cpu_line, "model name", 10) == 0) {
                        char *colon = strchr(cpu_line, ':');
                        if (colon) {
                                char* start = colon + 2; /* Skip colon and space */
                                char* end = strchr(start, '\n');
                                if (end) *end = '\0';
                                strncpy(cpu->name, start, sizeof(cpu->name)-1);
                        }
                }
                else if (strncmp(cpu_line, "vendor_id", 9) == 0) {
                        char *colon = strchr(cpu_line, ':');
                        if (colon) {
                                char* start = colon + 2;
                                char* end = strchr(start, '\n');
                                if (end) *end = '\0';
                                strncpy(cpu->vendor, start, sizeof(cpu->vendor)-1);
                        }
                }
                else if (strncmp(cpu_line, "cpu cores", 9) == 0) {
                        char *colon = strchr(cpu_line, ':');
                        if (colon) {
                                cpu->cores = atoi(colon + 2);
                        }
                }
                else if (strncmp(cpu_line, "siblings", 8) == 0) {
                        char *colon = strchr(cpu_line, ':');
                        if (colon) {
                                cpu->threads = atoi(colon + 2);
                        }
                }
        }

        fclose(f);

        /* Get architecture */
        struct utsname uts;
        if (uname(&uts) == 0) {
                strncpy(cpu->architecture, uts.machine, sizeof(cpu->architecture)-1);
        }

        return 1;
}

int
hardware_memory_info(HardwareMemory* mem)
{
        if (!mem) {
                return 0;
        }

        memset(mem, 0, sizeof(HardwareMemory));

        FILE *f = fopen("/proc/meminfo", "r");
        if (!f) {
                return 0;
        }

        char key[64], unit[64];
        unsigned long val;

        while (fscanf(f, "%63s %lu %63s", key, &val, unit) == 3) {
                if (strcmp(key, "MemTotal:") == 0) {
                        mem->total_phys = val * 1024; /* Convert KB to bytes */
                }
                else if (strcmp(key, "MemAvailable:") == 0) {
                        mem->avail_phys = val * 1024;
                }
        }

        fclose(f);
        return 1;
}

int
hardware_disk_info(HardwareDisk* disk, const char* mount_point)
{
        if (!disk || !mount_point) {
                return 0;
        }

        memset(disk, 0, sizeof(HardwareDisk));
        strncpy(disk->mount_point, mount_point, sizeof(disk->mount_point)-1);

        struct statvfs stat;
        if (statvfs(mount_point, &stat) == 0) {
                disk->total_bytes = (unsigned long long)stat.f_blocks * stat.f_frsize;
                disk->free_bytes = (unsigned long long)stat.f_bfree * stat.f_frsize;
                disk->used_bytes = disk->total_bytes - disk->free_bytes;

                disk->total_gb = disk->total_bytes / (1024.0 * 1024 * 1024);
                disk->free_gb = disk->free_bytes / (1024.0 * 1024 * 1024);
                disk->used_gb = disk->used_bytes / (1024.0 * 1024 * 1024);

                strncpy(disk->filesystem, "Unknown", sizeof(disk->filesystem)-1);

                return 1;
        }

        return 0;
}

#endif


/* ============================================================================
 * Comprehensive Display Functions
 * ==========================================================================*/

void
hardware_display_cpu_comprehensive(void)
{
        HardwareCPU cpu;
        if (hardware_cpu_info(&cpu)) {
                hardware_display_field(FIELD_CPU_NAME, "%s", cpu.name);
                hardware_display_field(FIELD_CPU_VENDOR, "%s", cpu.vendor);
                hardware_display_field(FIELD_CPU_ARCH, "%s", cpu.architecture);
                hardware_display_field(FIELD_CPU_CORES, "%u", cpu.cores);
                hardware_display_field(FIELD_CPU_THREADS, "%u", cpu.threads);
                if (cpu.clock_speed > 0) {
                        hardware_display_field(FIELD_CPU_CLOCK, "%.2f GHz", cpu.clock_speed);
                }
        }
}

void
hardware_display_memory_comprehensive(void)
{
        HardwareMemory mem;
        if (hardware_memory_info(&mem)) {
                hardware_display_field(FIELD_MEM_TOTAL, "%.2f GB",
                                                          mem.total_phys / (1024.0 * 1024 * 1024));
                hardware_display_field(FIELD_MEM_AVAIL, "%.2f GB",
                                                          mem.avail_phys / (1024.0 * 1024 * 1024));
                if (mem.speed > 0) {
                        hardware_display_field(FIELD_MEM_SPEED, "%u MHz", mem.speed);
                }
                if (strlen(mem.type) > 0) {
                        hardware_display_field(FIELD_MEM_TYPE, "%s", mem.type);
                }
        }
}

void
hardware_display_disk_comprehensive(const char* drive)
{
        HardwareDisk disk;
        if (hardware_disk_info(&disk, drive)) {
                hardware_display_field(FIELD_DISK_MOUNT, "%s", disk.mount_point);
                hardware_display_field(FIELD_DISK_FS, "%s", disk.filesystem);
                hardware_display_field(FIELD_DISK_TOTAL, "%.2f GB", disk.total_gb);
                hardware_display_field(FIELD_DISK_USED, "%.2f GB", disk.used_gb);
                hardware_display_field(FIELD_DISK_FREE, "%.2f GB", disk.free_gb);
        }
}


/* ============================================================================
 * Query and Summary Functions
 * ==========================================================================*/

void
hardware_query_specific(unsigned int* fields, int count)
{
        for (int i = 0; i < count; i++) {
                switch (fields[i]) {
                        case FIELD_CPU_NAME: 
                        case FIELD_CPU_VENDOR: 
                        case FIELD_CPU_CORES: 
                        case FIELD_CPU_THREADS:
                        case FIELD_CPU_ARCH:
                        {
                                static int cpu_loaded = 0;
                                static HardwareCPU cpu;
                                if (!cpu_loaded) {
                                        hardware_cpu_info(&cpu);
                                        cpu_loaded = 1;
                                }

                                switch (fields[i]) {
                                        case FIELD_CPU_NAME:
                                                hardware_display_field(FIELD_CPU_NAME, "%s", cpu.name);
                                                break;
                                        case FIELD_CPU_VENDOR:
                                                hardware_display_field(FIELD_CPU_VENDOR, "%s", cpu.vendor);
                                                break;
                                        case FIELD_CPU_CORES:
                                                hardware_display_field(FIELD_CPU_CORES, "%u", cpu.cores);
                                                break;
                                        case FIELD_CPU_THREADS:
                                                hardware_display_field(FIELD_CPU_THREADS, "%u", cpu.threads);
                                                break;
                                        case FIELD_CPU_ARCH:
                                                hardware_display_field(FIELD_CPU_ARCH, "%s", cpu.architecture);
                                                break;
                                }
                        }
                        break;

                        case FIELD_MEM_TOTAL: 
                        case FIELD_MEM_AVAIL:
                        {
                                static int mem_loaded = 0;
                                static HardwareMemory mem;
                                if (!mem_loaded) {
                                        hardware_memory_info(&mem);
                                        mem_loaded = 1;
                                }

                                switch (fields[i]) {
                                        case FIELD_MEM_TOTAL:
                                                hardware_display_field(FIELD_MEM_TOTAL, "%.2f GB",
                                                                                          mem.total_phys / (1024.0 * 1024 * 1024));
                                                break;
                                        case FIELD_MEM_AVAIL:
                                                hardware_display_field(FIELD_MEM_AVAIL, "%.2f GB",
                                                                                          mem.avail_phys / (1024.0 * 1024 * 1024));
                                                break;
                                }
                        }
                        break;

                        case FIELD_DISK_FREE: 
                        case FIELD_DISK_TOTAL:
                        {
                                static int disk_loaded = 0;
                                static HardwareDisk disk;
                                if (!disk_loaded) {
#ifdef WG_WINDOWS
                                        hardware_disk_info(&disk, "C:\\");
#else
                                        hardware_disk_info(&disk, "/");
#endif
                                        disk_loaded = 1;
                                }

                                switch (fields[i]) {
                                        case FIELD_DISK_TOTAL:
                                                hardware_display_field(FIELD_DISK_TOTAL, "%.2f GB", disk.total_gb);
                                                break;
                                        case FIELD_DISK_FREE:
                                                hardware_display_field(FIELD_DISK_FREE, "%.2f GB", disk.free_gb);
                                                break;
                                }
                        }
                        break;
                }
        }
}

void
hardware_show_summary(void)
{
        printf("=== HARDWARE SUMMARY ===\n");
        hardware_display_cpu_comprehensive();
        hardware_display_memory_comprehensive();

#ifdef WG_WINDOWS
        hardware_display_disk_comprehensive("C:\\");
#else
        hardware_display_disk_comprehensive("/");
#endif

        printf("\n");
}

void
hardware_show_detailed(void)
{
        printf("=== DETAILED HARDWARE INFORMATION ===\n");

        /* CPU Section */
        printf("\n[CPU Information]\n");
        hardware_display_cpu_comprehensive();

        /* Memory Section */
        printf("\n[Memory Information]\n");
        hardware_display_memory_comprehensive();

        /* Disk Section */
        printf("\n[Disk Information]\n");
#ifdef WG_WINDOWS
        hardware_display_disk_comprehensive("C:\\");
#else
        hardware_display_disk_comprehensive("/");
#endif
}


/* ============================================================================
 * Legacy Compatibility Functions
 * ==========================================================================*/

void
hardware_cpu_info_legacy(void)
{
        hardware_display_cpu_comprehensive();
}

void
hardware_memory_info_legacy(void)
{
        hardware_display_memory_comprehensive();
}

void
hardware_disk_info_legacy(void)
{
#ifdef WG_WINDOWS
        hardware_display_disk_comprehensive("C:\\");
#else
        hardware_display_disk_comprehensive("/");
#endif
}

void
hardware_network_info_legacy(void)
{
        /* Basic network info implementation */
        printf("%-15s: Network information\n", "Network");
}

void
hardware_system_info_legacy(void)
{
        /* Basic system info implementation */
#ifdef WG_WINDOWS
        printf("%-15s: Windows\n", "OS");
#else
        struct utsname uts;
        uname(&uts);
        printf("%-15s: %s %s\n", "OS", uts.sysname, uts.release);
#endif
}