#ifndef HARDWARE_H
#define HARDWARE_H

#ifdef _WIN32
void get_cpu_info();
void get_memory_info();
void get_disk_info();
void get_network_info();
void get_system_info();
#else
void get_cpu_info();
void get_memory_info();
void get_disk_info();
void get_network_info();
void get_system_info();
void get_bios_info();
void get_gpu_info();
#endif

#endif