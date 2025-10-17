#ifndef SERVER_H
#define SERVER_H

void watchdogs_server_stop_tasks(void);
void watchdogs_server_samp(const char *gamemode_arg, const char *server_bin);
void watchdogs_server_openmp(const char *gamemode_arg, const char *server_bin);

#endif