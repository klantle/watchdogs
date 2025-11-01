#ifndef SERVER_H
#define SERVER_H

void wd_stop_server_tasks(void);
void wd_display_server_logs(int ret);
void restore_samp_config(void);
void restore_omp_config(void);
void wd_run_samp_server(const char *gamemode_arg, const char *server_bin);
void wd_run_omp_server(const char *gamemode_arg, const char *server_bin);

#endif
