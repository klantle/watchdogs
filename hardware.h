#ifndef HARDWARE_H
#define HARDWARE_H

void hardware_cpu_info();
void hardware_memory_info();
void hardware_disk_info();
void hardware_network_info();
void hardware_system_info();

#ifndef _WIN32
void hardware_bios_info();
void hardware_gpu_info();
#endif

#endif