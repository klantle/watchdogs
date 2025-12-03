#ifndef HARDWARE_INFO_H
#define HARDWARE_INFO_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "utils.h"

#if defined(WG_WINDOWS) || defined(_WIN64)
#define HARDWARE_WINDOWS 1
#define HARDWARE_UNIX 0
#else
#define HARDWARE_WINDOWS 0
#define HARDWARE_UNIX 1
#endif

#define hardware_cpuid __cpuid

#define cpuid_cpuinfo  0x80000000
#define cpuid_brand    0x80000002
#define cpuid_brand_16 0x80000003
#define cpuid_brand_32 0x80000004
#define cpu_maxid      0x80000004

#define FIELD_CPU_NAME          0x0001
#define FIELD_CPU_VENDOR        0x0002
#define FIELD_CPU_CORES         0x0003
#define FIELD_CPU_THREADS       0x0004
#define FIELD_CPU_CLOCK         0x0005
#define FIELD_CPU_CACHE         0x0006
#define FIELD_CPU_ARCH          0x0007

#define FIELD_MEM_TOTAL         0x0101
#define FIELD_MEM_AVAIL         0x0102
#define FIELD_MEM_SPEED         0x0103
#define FIELD_MEM_TYPE          0x0104

#define FIELD_DISK_MOUNT        0x0201
#define FIELD_DISK_TOTAL        0x0202
#define FIELD_DISK_FREE         0x0203
#define FIELD_DISK_USED         0x0204
#define FIELD_DISK_FS           0x0205

#define FIELD_NET_NAME          0x0301
#define FIELD_NET_MAC           0x0302
#define FIELD_NET_IPV4          0x0303
#define FIELD_NET_IPV6          0x0304
#define FIELD_NET_SPEED         0x0305
#define FIELD_NET_MTU           0x0306

#define FIELD_GPU_NAME          0x0401
#define FIELD_GPU_VENDOR        0x0402
#define FIELD_GPU_MEMORY        0x0403

#define FIELD_BIOS_VENDOR       0x0501
#define FIELD_BIOS_VERSION      0x0502
#define FIELD_BIOS_DATE         0x0503

typedef struct {
        char name[128];
        char vendor[64];
        char architecture[32];
        unsigned int cores;
        unsigned int threads;
        double clock_speed;
        unsigned int cache_l1;
        unsigned int cache_l2;
        unsigned int cache_l3;
} HardwareCPU;

typedef struct {
        unsigned long long total_phys;
        unsigned long long avail_phys;
        unsigned long long total_virt;
        unsigned long long avail_virt;
        unsigned int speed;
        char type[32];
} HardwareMemory;

typedef struct {
        char mount_point[256];
        char filesystem[32];
        unsigned long long total_bytes;
        unsigned long long free_bytes;
        unsigned long long used_bytes;
        double total_gb;
        double free_gb;
        double used_gb;
} HardwareDisk;

typedef struct {
        char name[64];
        char description[128];
        char mac_address[18];
        char ipv4[16];
        char ipv6[46];
        unsigned long long speed;
        unsigned int mtu;
} HardwareNetwork;

#ifdef WG_WINDOWS
struct utsname {
        char sysname[256];
        char nodename[256];
        char release[256];
        char version[256];
        char machine[256];
};
int uname(struct utsname *name);
#endif

int hardware_cpu_info(HardwareCPU* cpu);
int hardware_memory_info(HardwareMemory* mem);
int hardware_disk_info(HardwareDisk* disk, const char* mount_point);

void hardware_display_field(unsigned int field_id, const char* format, ...);
void hardware_display_cpu_comprehensive(void);
void hardware_display_memory_comprehensive(void);
void hardware_display_disk_comprehensive(const char* mount_point);

void hardware_query_specific(unsigned int* fields, int count);

void hardware_show_summary(void);
void hardware_show_detailed(void);

void hardware_cpu_info_legacy(void);
void hardware_memory_info_legacy(void);
void hardware_disk_info_legacy(void);
void hardware_network_info_legacy(void);
void hardware_system_info_legacy(void);

#ifdef __cplusplus
}
#endif

#endif
