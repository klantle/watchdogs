#ifndef SERVER_H
#define SERVER_H

void wd_StopServerTasks(void);
void wd_RunSAMPServer(const char *gamemode_arg, const char *server_bin);
void wd_RunOMPServer(const char *gamemode_arg, const char *server_bin);

#endif
