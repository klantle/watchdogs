#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "hardware.h"

#ifdef _WIN32

#include <windows.h>
#include <iphlpapi.h>
#include <intrin.h>
#pragma comment(lib, "iphlpapi.lib")

void get_cpu_info() {
        int cpuInfo[4] = {0};
        char brand[64];
        __cpuid(cpuInfo, 0x80000000);
        unsigned int maxId = cpuInfo[0];

        memset(brand, 0, sizeof(brand));
        if (maxId >= 0x80000004) {
            __cpuid((int*)(brand),    0x80000002);
            __cpuid((int*)(brand+16), 0x80000003);
            __cpuid((int*)(brand+32), 0x80000004);
            printf("CPU: %s\n", brand);
        }

        SYSTEM_INFO si;
        GetSystemInfo(&si);
        printf("Cores: %lu\n", si.dwNumberOfProcessors);
}

void get_memory_info() {
        MEMORYSTATUSEX memInfo;
        memInfo.dwLength = sizeof(memInfo);
        GlobalMemoryStatusEx(&memInfo);
        printf("Total RAM: %.2f GB\n", memInfo.ullTotalPhys / (1024.0 * 1024 * 1024));
}

void get_disk_info() {
        ULARGE_INTEGER freeBytes, totalBytes, totalFree;
        if (GetDiskFreeSpaceEx("C:\\", &freeBytes, &totalBytes, &totalFree))
            printf("Disk C: %.2f GB total, %.2f GB free\n",
                totalBytes.QuadPart / (1024.0 * 1024 * 1024),
                freeBytes.QuadPart  / (1024.0 * 1024 * 1024));
}

void get_network_info() {
        DWORD dwSize = 0;
        GetAdaptersInfo(NULL, &dwSize);
        PIP_ADAPTER_INFO pAdapterInfo = (IP_ADAPTER_INFO *)malloc(dwSize);
        if (GetAdaptersInfo(pAdapterInfo, &dwSize) == NO_ERROR) {
            PIP_ADAPTER_INFO pAdapter = pAdapterInfo;
            while (pAdapter) {
                printf("Interface: %s\n", pAdapter->Description);
                printf("  MAC: %02X-%02X-%02X-%02X-%02X-%02X\n",
                    pAdapter->Address[0], pAdapter->Address[1],
                    pAdapter->Address[2], pAdapter->Address[3],
                    pAdapter->Address[4], pAdapter->Address[5]);
                pAdapter = pAdapter->Next;
            }
        }
        free(pAdapterInfo);
}

void get_system_info() {
        OSVERSIONINFOEX osvi = {0};
        osvi.dwOSVersionInfoSize = sizeof(osvi);
        GetVersionEx((OSVERSIONINFO *)&osvi);
        printf("OS: Windows %lu.%lu build %lu\n", osvi.dwMajorVersion,
                                                  osvi.dwMinorVersion,
                                                  osvi.dwBuildNumber);
}

#else

#include <sys/statvfs.h>
#include <ifaddrs.h>
#include <net/if.h>
#include <unistd.h>
#include <sys/utsname.h>

void get_cpu_info() {
        FILE *f = fopen("/proc/cpuinfo", "r");
        if (!f) return;
        char line[256];
        while (fgets(line, sizeof(line), f)) {
            if (strncmp(line, "model name", 10) == 0) {
                printf("%s", line);
                break;
            }
        }
        fclose(f);
}

void get_memory_info() {
        FILE *f = fopen("/proc/meminfo", "r");
        if (!f) return;
        char key[64], unit[64];
        unsigned long val;
        while (fscanf(f, "%63s %lu %63s", key, &val, unit) == 3) {
            if (strcmp(key, "MemTotal:") == 0) {
                printf("Total RAM: %.2f GB\n", val / (1024.0 * 1024));
                break;
            }
        }
        fclose(f);
}

void get_disk_info() {
        struct statvfs stat;
        if (statvfs("/", &stat) == 0) {
            double total = (double)stat.f_blocks * stat.f_frsize / (1024 * 1024 * 1024);
            double free  = (double)stat.f_bfree * stat.f_frsize  / (1024 * 1024 * 1024);
            printf("Disk: %.2f GB total, %.2f GB free\n", total, free);
        }
}

void get_network_info() {
        struct ifaddrs *ifaddr;
        getifaddrs(&ifaddr);
        for (struct ifaddrs *ifa = ifaddr; ifa; ifa = ifa->ifa_next) {
            if (ifa->ifa_addr && ifa->ifa_addr->sa_family == AF_PACKET)
                printf("Interface: %s\n", ifa->ifa_name);
        }
        freeifaddrs(ifaddr);
}

void get_system_info() {
        struct utsname uts;
        uname(&uts);
        printf("OS: %s %s %s\n", uts.sysname, uts.release, uts.machine);
}

void get_bios_info() {
        FILE *f = fopen("/sys/class/dmi/id/board_vendor", "r");
        if (f) {
            char buf[128];
            if (fgets(buf, sizeof(buf), f))
            printf("Motherboard: %s", buf);
            fclose(f);
        }
}

void get_gpu_info() {
        FILE *f = popen("lspci | grep VGA", "r");
        if (f) {
            char buf[256];
            if (fgets(buf, sizeof(buf), f))
            printf("GPU: %s", buf);
            pclose(f);
        }
}

#endif