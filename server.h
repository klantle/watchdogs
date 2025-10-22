#ifndef SERVER_H
#define SERVER_H

void watch_server_stop_tasks(void);
void watch_server_samp(const char *gamemode_arg, const char *server_bin);
void watch_server_openmp(const char *gamemode_arg, const char *server_bin);

#endif
