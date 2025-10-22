#ifndef WATCHDOGS_H
#define WATCHDOGS_H

extern int __os__;
extern const char *ptr_samp;
extern int find_for_samp;
extern const char *ptr_openmp;
extern int find_for_omp;

void __init(int sig_unused);

#endif