#ifndef _GNU_SOURCE
    #define _GNU_SOURCE
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdbool.h>
#include <limits.h>
#include <dirent.h>
#include <time.h>
#include <ftw.h>
#include <fcntl.h>
#include <math.h>
#include <locale.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <signal.h>
#include <curl/curl.h>

#include "extra.h"
#include "utils.h"
#include "crypto.h"
#include "library.h"
#include "archive.h"
#include "curl.h"
#include "runner.h"
#include "compiler.h"
#include "replicate.h"
#include "debug.h"
#include "units.h"

/*  source
    ├── archive.c
    ├── archive.h
    ├── cause.c
    ├── cause.h
    ├── compiler.c
    ├── compiler.h
    ├── crypto.c
    ├── crypto.h
    ├── curl.c
    ├── curl.h
    ├── debug.c
    ├── debug.h
    ├── extra.c
    ├── extra.h
    ├── library.c
    ├── library.h
    ├── replicate.c
    ├── replicate.h
    ├── runner.c
    ├── runner.h
    ├── units.c [x]
    ├── units.h
    ├── utils.c
    └── utils.h
*/

#if defined(__W_VERSION__)
  #define WATCHDOGS_RELEASE __W_VERSION__
#else
  #define WATCHDOGS_RELEASE "WATCHDOGS"
#endif
const char *watchdogs_release=WATCHDOGS_RELEASE;

static struct timespec cmd_start, cmd_end={ 0 };
static double command_dur;
int dog_ptr_command_init=0;

int __command__(char *unit_pre_command)
{
  __create_unit_logging(1);

  int dog_compile_running=0;
  char *ptr_prompt=NULL;
  size_t size_ptr_command=DOG_MAX_PATH+DOG_PATH_MAX;
  char *ptr_command=NULL;
  const char *command_similar;
  int dist=INT_MAX;

  ptr_prompt=dog_malloc(size_ptr_command);
  if(!ptr_prompt) return -1;

_ptr_command:
  if(ptr_command) {
    free(ptr_command);
    ptr_command=NULL;
  }
  if(unit_pre_command && unit_pre_command[0]!='\0') {
    ptr_command=strdup(unit_pre_command);
    if(strfind(ptr_command,"0000WGDEBUGGINGSERVER",true)==0)
      printf("~%s\n",ptr_command);
  } else {
    static int k=0;
    if(k!=1) {
      ++k;
      ptr_command=strdup("dog");
      printf("~%s\n",ptr_command);
      goto _reexecute_command;
    }
    while(true) {
        snprintf(ptr_prompt,size_ptr_command,
                "~");
        char *input=readline(ptr_prompt);
        if(input==NULL) {
            free(input);
            return 2;
        }
        if(input[0]=='\0') {
            free(input);
            continue;
        }
        ptr_command=strdup(input);
        free(input);
        break;
    }
  }

  fflush(stdout);
  if(ptr_command && ptr_command[0]!='\0') {
      HIST_ENTRY *last=history_get(history_length);
      if(last == NULL ||
          strcmp(last->line, ptr_command)!=0)
        {
          dog_a_history(ptr_command);
        }
  }

  dog_a_history(ptr_command);

  command_similar=dog_find_near_command(ptr_command,__command,__command_len,&dist);

_reexecute_command:
  ++dog_ptr_command_init;
  __create_unit_logging(0);
  clock_gettime(CLOCK_MONOTONIC,&cmd_start);
  if(strncmp(ptr_command,"help",strlen("help"))==0) {
    dog_console_title("Watchdogs | @ help");

    char *args;
    args=ptr_command+strlen("help");
    while(*args==' ') ++args;

    unit_show_help(args);

    goto unit_done;
  } else if(strcmp(ptr_command,"exit")==0) {
    dog_free(ptr_command);
    ptr_command=NULL;
    dog_free(ptr_prompt);
    ptr_prompt=NULL;
    return 2;
  } else if(strcmp(ptr_command,"kill")==0) {
    dog_console_title("Watchdogs | @ kill");

    dog_clear_screen();

    wgconfig.dog_sel_stat=0;
    dog_compile_running=0;

    __create_unit_logging(1);

    if(unit_pre_command && unit_pre_command[0]!='\0')
      goto unit_done;
    else
      goto _ptr_command;
  } else if(strncmp(ptr_command,"title",strlen("title"))==0) {
    char *args=ptr_command+strlen("title");
    while(*args==' ') ++args;

    if(*args=='\0') {
      println(stdout,"Usage: title [<title>]");
    } else {
      size_t title_len=strlen(args)+1;
      char *title_set=dog_malloc(title_len);
      if(!title_set) goto unit_done;
      snprintf(title_set,title_len,"%s",args);
      dog_console_title(title_set);
      dog_free(title_set);
      title_set=NULL;
    }

    goto unit_done;
  } else if(strncmp(ptr_command,"sha1",strlen("sha1"))==0) {
    char *args=ptr_command+strlen("sha1");
    while(*args==' ') ++args;

    if(*args=='\0') {
      println(stdout,"Usage: sha1 [<words>]");
    } else {
      unsigned char digest[20];

      if(crypto_generate_sha1_hash(args,digest)!=1) {
        goto unit_done;
      }

      printf("        Crypto Input : %s\n",args);
      printf("Crypto Output (sha1) : ");
      crypto_print_hex(digest,sizeof(digest),1);
    }

    goto unit_done;
  } else if(strncmp(ptr_command,"sha256",strlen("sha256"))==0) {
    char *args=ptr_command+strlen("sha256");
    while(*args==' ') ++args;

    if(*args=='\0') {
      println(stdout,"Usage: sha256 [<words>]");
    } else {
      unsigned char digest[32];

      if(crypto_generate_sha256_hash(args,digest)!=1) {
        goto unit_done;
      }

      printf("          Crypto Input : %s\n",args);
      printf("Crypto Output (SHA256) : ");
      crypto_print_hex(digest,sizeof(digest),1);
    }

    goto unit_done;
  } else if(strncmp(ptr_command,"crc32",strlen("crc32"))==0) {
    char *args=ptr_command+strlen("crc32");
    while(*args==' ') ++args;

    if(*args=='\0') {
      println(stdout,"Usage: crc32 [<words>]");
    } else {
      static int init_crc32=0;
      if(init_crc32!=1) {
        crypto_crc32_init_table();
        init_crc32=1;
      }

      uint32_t crc32_generate;
      crc32_generate=crypto_generate_crc32(args,strlen(args));

      char crc_str[11];
      sprintf(crc_str,"%08X",crc32_generate);

      printf("         Crypto Input : %s\n",args);
      printf("Crypto Output (CRC32) : ");
      printf("%s\n",crc_str);
    }

    goto unit_done;
  } else if(strncmp(ptr_command,"djb2",strlen("djb2"))==0) {
    char *args=ptr_command+strlen("djb2");
    while(*args==' ') ++args;

    if(*args=='\0') {
      println(stdout,"Usage: djb2 [<file>]");
    } else {
      if(path_exists(args)==0) {
        pr_error(stdout,"djb2: " FCOLOUR_CYAN "%s" " - No such file or directory",args);
        goto unit_done;
      }
      unsigned long djb2_generate;
      djb2_generate=crypto_djb2_hash_file(args);

      if(djb2_generate) {
        printf("        Crypto Input : %s\n",args);
        printf("Crypto Output (DJB2) : ");
        printf("%#lx\n",djb2_generate);
      }
    }

    goto unit_done;
  } else if(strcmp(ptr_command,"config")==0) {
    if(access("watchdogs.toml",F_OK)==0)
      remove("watchdogs.toml");

    __create_unit_logging(1);

    printf(FCOLOUR_B_BLUE "");
    dog_printfile("watchdogs.toml");
    printf(FCOLOUR_DEFAULT "\n");

    goto unit_done;
  } else if(strncmp(ptr_command,"replicate",strlen("replicate"))==0) {
    dog_console_title("Watchdogs | @ replicate depends");
    char *args=ptr_command+strlen("replicate");
    while(*args==' ') ++args;

    int is_null_args=-1;
    if(args[0]=='\0' || strlen(args)<1)
      is_null_args=1;

    char *__args=strdup(args);
    char *raw_branch=NULL;
    char *raw_save=NULL;

    char *args2=strtok(__args," ");
    if(args2==NULL || strcmp(args2,".")==0)
      is_null_args=1;

    char *procure_args=strtok(args," ");
    while(procure_args) {
      if(strcmp(procure_args,"--branch")==0) {
        procure_args=strtok(NULL," ");
        if(procure_args) raw_branch=procure_args;
      } else if(strcmp(procure_args,"--save")==0) {
        procure_args=strtok(NULL," ");
        if(procure_args) raw_save=procure_args;
      }
      procure_args=strtok(NULL," ");
    }

    if(raw_save && strcmp(raw_save,".")==0) {
      static char *fetch_pwd=NULL;
      fetch_pwd=dog_procure_pwd();
      raw_save=strdup(fetch_pwd);
    }

    dog_free(__args);

    if(is_null_args!=1) {
      if(raw_branch && raw_save)
        dog_install_depends(args,raw_branch,raw_save);
      else if(raw_branch)
        dog_install_depends(args,raw_branch,NULL);
      else if(raw_save)
        dog_install_depends(args,"main",raw_save);
      else
        dog_install_depends(args,"main",NULL);
    } else {
      char errbuf[DOG_PATH_MAX];
      toml_table_t *dog_toml_config;
      FILE *this_proc_file=fopen("watchdogs.toml","r");
      dog_toml_config=toml_parse_file(this_proc_file,errbuf,sizeof(errbuf));
      if(this_proc_file) fclose(this_proc_file);

      if(!dog_toml_config) {
        pr_error(stdout,"failed to parse the watchdogs.toml...: %s",errbuf);
        __create_logging();
        return 0;
      }

      toml_table_t *dog_depends;
      size_t arr_sz,i;
      char *expect=NULL;

      dog_depends=toml_table_in(dog_toml_config,"dependencies");
      if(!dog_depends)
        goto out;

      toml_array_t *dog_toml_packages=toml_array_in(dog_depends,"packages");
      if(!dog_toml_packages)
        goto out;

      arr_sz=toml_array_nelem(dog_toml_packages);
      for(i=0;i<arr_sz;i++) {
        toml_datum_t val;

        val=toml_string_at(dog_toml_packages,i);
        if(!val.ok)
          continue;

        if(!expect) {
          expect=dog_realloc(NULL,strlen(val.u.s)+1);
          if(!expect)
            goto free_val;

          snprintf(expect,strlen(val.u.s)+1,"%s",val.u.s);
        } else {
          char *tmp;
          size_t old_len=strlen(expect);
          size_t new_len=old_len+strlen(val.u.s)+2;

          tmp=dog_realloc(expect,new_len);
          if(!tmp)
            goto free_val;

          expect=tmp;
          snprintf(expect+old_len,new_len-old_len," %s",val.u.s);
        }

free_val:
        dog_free(val.u.s);
        val.u.s=NULL;
      }

      if(!expect)
        expect=strdup("");

      dog_free(wgconfig.dog_toml_packages);
      wgconfig.dog_toml_packages=expect;

      pr_info(stdout,"Trying install packages: %s",wgconfig.dog_toml_packages);

      if(raw_branch && raw_save)
        dog_install_depends(wgconfig.dog_toml_packages,raw_branch,raw_save);
      else if(raw_branch)
        dog_install_depends(wgconfig.dog_toml_packages,raw_branch,NULL);
      else if(raw_save)
        dog_install_depends(wgconfig.dog_toml_packages,"main",raw_save);
      else
        dog_install_depends(wgconfig.dog_toml_packages,"main",NULL);
out:
      toml_free(dog_toml_config);
    }

    goto unit_done;
  } else if(strcmp(ptr_command,"gamemode")==0) {
    dog_console_title("Watchdogs | @ gamemode");
ret_ptr:
    printf("\033[1;33m== Select a Platform ==\033[0m\n");
    printf("  \033[36m[l]\033[0m Linux\n");
    printf("  \033[36m[w]\033[0m Windows\n  ^ \033[90m(Supported for: WSL/WSL2 ; not: Docker or Podman on WSL)\033[0m\n");

    wgconfig.dog_sel_stat=1;

    char *platform=readline("==> ");

    if(strfind(platform,"L",true)) {
      free(ptr_command);
      ptr_command=NULL;
      free(platform);
      int ret=dog_install_server("linux");
loop_igm:
      if(ret==-1 && wgconfig.dog_sel_stat!=0)
        goto loop_igm;
      else if(ret==0)
      {
        if(unit_pre_command && unit_pre_command[0]!='\0')
          goto unit_done;
        else
          goto _ptr_command;
      }
    } else if(strfind(platform,"W",true)) {
      free(ptr_command);
      ptr_command=NULL;
      free(platform);
      int ret=dog_install_server("windows");
loop_igm2:
      if(ret==-1 && wgconfig.dog_sel_stat!=0)
        goto loop_igm2;
      else if(ret==0)
      {
        if(unit_pre_command && unit_pre_command[0]!='\0')
          goto unit_done;
        else
          goto _ptr_command;
      }
    } else if(strfind(platform,"E",true)) {
      free(ptr_command);
      ptr_command=NULL;
      free(platform);
      if(unit_pre_command && unit_pre_command[0]!='\0')
        goto unit_done;
      else
        goto _ptr_command;
    } else {
      pr_error(stdout,"Invalid platform selection. Input 'E/e' to exit");
      free(platform);
      platform=NULL;
      goto ret_ptr;
    }

    goto unit_done;
  } else if(strcmp(ptr_command,"pawncc")==0) {
    dog_console_title("Watchdogs | @ pawncc");
ret_ptr2:
    printf("\033[1;33m== Select a Platform ==\033[0m\n");
    printf("  \033[36m[l]\033[0m Linux\n");
    printf("  \033[36m[w]\033[0m Windows\n  ^ \033[90m(Supported for: WSL/WSL2 ; not: Docker or Podman on WSL)\033[0m\n");
    printf("  \033[36m[t]\033[0m Termux\n");

    wgconfig.dog_sel_stat=1;

    char *platform=readline("==> ");

    if(strfind(platform,"L",true)) {
      free(ptr_command);
      ptr_command=NULL;
      free(platform);
      int ret=dog_install_pawncc("linux");
loop_ipcc:
      if(ret==-1 && wgconfig.dog_sel_stat!=0)
        goto loop_ipcc;
      else if(ret==0)
      {
        if(unit_pre_command && unit_pre_command[0]!='\0')
          goto unit_done;
        else
          goto _ptr_command;
      }
    } else if(strfind(platform,"W",true)) {
      free(ptr_command);
      ptr_command=NULL;
      free(platform);
      int ret=dog_install_pawncc("windows");
loop_ipcc2:
      if(ret==-1 && wgconfig.dog_sel_stat!=0)
        goto loop_ipcc2;
      else if(ret==0)
      {
        if(unit_pre_command && unit_pre_command[0]!='\0')
          goto unit_done;
        else
          goto _ptr_command;
      }
    } else if(strfind(platform,"T",true)) {
      free(ptr_command);
      ptr_command=NULL;
      free(platform);
      int ret=dog_install_pawncc("termux");
loop_ipcc3:
      if(ret==-1 && wgconfig.dog_sel_stat!=0)
        goto loop_ipcc3;
      else if(ret==0)
      {
        if(unit_pre_command && unit_pre_command[0]!='\0')
          goto unit_done;
        else
          goto _ptr_command;
      }
    } else if(strfind(platform,"E",true)) {
      free(ptr_command);
      ptr_command=NULL;
      free(platform);
      if(unit_pre_command && unit_pre_command[0]!='\0')
        goto unit_done;
      else
        goto _ptr_command;
    } else {
      pr_error(stdout,"Invalid platform selection. Input 'E/e' to exit");
      free(platform);
      platform=NULL;
      goto ret_ptr2;
    }

    goto unit_done;
  } else if(strcmp(ptr_command,"debug")==0) {
    dog_console_title("Watchdogs | @ debug");
    #ifdef DOG_ANDROID
      #ifndef _DBG_PRINT
        dog_exec_command("./watchdogs.tmux 0000WGDEBUGGINGSERVER");
      #else
        dog_exec_command("./watchdogs.debug.tmux 0000WGDEBUGGINGSERVER");
      #endif
    #elif defined(DOG_LINUX)
      #ifndef _DBG_PRINT
        dog_exec_command("./watchdogs 0000WGDEBUGGINGSERVER");
      #else
        dog_exec_command("./watchdogs.debug 0000WGDEBUGGINGSERVER");
      #endif
    #elif defined(DOG_WINDOWS)
      #ifndef _DBG_PRINT
        dog_exec_command("watchdogs.win 0000WGDEBUGGINGSERVER");
      #else
        dog_exec_command("watchdogs.debug.win 0000WGDEBUGGINGSERVER");
      #endif
    #endif
    goto unit_done;
  } else if(strcmp(ptr_command,"0000WGDEBUGGINGSERVER")==0) {
    dog_server_crash_check();
    dog_free(ptr_command);
    ptr_command=NULL;
    return 3;
  } else if(strncmp(ptr_command,"compile",strlen("compile"))==0 &&
            !isalpha((unsigned char)ptr_command[strlen("compile")])) {
    dog_console_title("Watchdogs | @ compile | logging file: .watchdogs/compiler.log");

    char *args;
    char *compile_args;
    char *second_arg=NULL;
    char *four_arg=NULL;
    char *five_arg=NULL;
    char *six_arg=NULL;
    char *seven_arg=NULL;
    char *eight_arg=NULL;
    char *nine_arg=NULL;

    args=ptr_command+strlen("compile");

    while(*args==' ') {
      args++;
    }

    compile_args=strtok(args," ");
    second_arg=strtok(NULL," ");
    four_arg=strtok(NULL," ");
    five_arg=strtok(NULL," ");
    six_arg=strtok(NULL," ");
    seven_arg=strtok(NULL," ");
    eight_arg=strtok(NULL," ");
    nine_arg=strtok(NULL," ");

    dog_exec_compiler(args,compile_args,second_arg,four_arg,
                    five_arg,six_arg,seven_arg,eight_arg,
                    nine_arg);

    goto unit_done;
  } else if(strncmp(ptr_command,"running",strlen("running"))==0) {
_runners_:
    dog_stop_server_tasks();

    if(is_termux_env()==1) {
      pr_warning(stdout,"Currently Termux is not supported..");
      pr_info(stdout,"Alternative's for Termux:"
             "\n\tOMP Termux (Beta): https://github.com/novusr/omptmux/releases/tag/v1.5.8.3079"
             "\n\tSA-MP decompilation (work-in-progress): http://github.com/dashr9230/SA-MP");
    }

    if(!path_access(wgconfig.dog_toml_binary)) {
      pr_error(stdout,"can't locate sa-mp/open.mp binary file!");
      goto unit_done;
    }
    if(!path_access(wgconfig.dog_toml_config)) {
      pr_warning(stdout,"can't locate %s - config file!",wgconfig.dog_toml_config);
      goto unit_done;
    }

    if(dir_exists(".watchdogs")==0)
      MKDIR(".watchdogs");

    int access_debugging_file=path_access(wgconfig.dog_toml_logs);
    if(access_debugging_file)
      remove(wgconfig.dog_toml_logs);
    access_debugging_file=path_access(wgconfig.dog_toml_logs);
    if(access_debugging_file)
      remove(wgconfig.dog_toml_logs);

    size_t cmd_len=7;
    char *args=ptr_command+cmd_len;
    while(*args==' ') ++args;
    char *args2=NULL;
    args2=strtok(args," ");

    char *size_arg1=NULL;
    if(args2==NULL || args2[0]=='\0')
      size_arg1=wgconfig.dog_toml_proj_output;
    else
      size_arg1=args2;

    size_t needed=snprintf(NULL,0, "Watchdogs | "
                           "@ running | " "args: %s | "
                           "config: %s | " "CTRL + C to stop. | \"debug\" for debugging",
                           size_arg1, wgconfig.dog_toml_config)+1;
    char *title_running_info=dog_malloc(needed);
    if(!title_running_info) { goto unit_done; }
    snprintf(title_running_info,needed,
             "Watchdogs | "
             "@ running | "
             "args: %s | "
             "config: %s | "
             "CTRL + C to stop. | \"debug\" for debugging",
             size_arg1,
             wgconfig.dog_toml_config);
    if(title_running_info) {
      #ifdef DOG_ANDROID
        println(stdout, "%s", title_running_info);
      #else
        dog_console_title(title_running_info);
      #endif
      dog_free(title_running_info);
      title_running_info=NULL;
    }

    int _dog_config_acces=path_access(wgconfig.dog_toml_config);
    if(!_dog_config_acces) {
      pr_error(stdout,"%s not found!",wgconfig.dog_toml_config);
      goto unit_done;
    }

    pr_color(stdout,FCOLOUR_YELLOW,"running..\n");
    println(stdout,"\toperating system: %s",wgconfig.dog_toml_os_type);
    println(stdout,"\tbinary file: %s",wgconfig.dog_toml_binary);
    println(stdout,"\tconfig file: %s",wgconfig.dog_toml_config);

    char *command=dog_malloc(DOG_PATH_MAX);
    if(!command) goto unit_done;
    struct sigaction sa;

    if(path_access("announce"))
      CHMOD_FULL("announce");

    if(dog_server_env()==1) {
      if(args2==NULL || (args2[0]=='.' && args2[1]=='\0')) {
start_main:
        sa.sa_handler=unit_sigint_handler;
        sigemptyset(&sa.sa_mask);
        sa.sa_flags=SA_RESTART;

        if(sigaction(SIGINT,&sa,NULL)==-1) {
          perror("sigaction");
          exit(EXIT_FAILURE);
        }

        time_t start,end;
        double elapsed;

        int ret_serv=0;
back_start:
        start=time(NULL);
        printf(FCOLOUR_B_BLUE "");
        #ifdef DOG_WINDOWS
          snprintf(command,DOG_PATH_MAX,"%s",wgconfig.dog_toml_binary);
        #else
          CHMOD_FULL(wgconfig.dog_toml_binary);
          snprintf(command,DOG_PATH_MAX,"./%s",wgconfig.dog_toml_binary);
        #endif
        int rate_runner_failed=dog_exec_command(command);
        if(rate_runner_failed==0) {
          if(!strcmp(wgconfig.dog_os_type,OS_SIGNAL_LINUX)) {
            printf(FCOLOUR_DEFAULT "\n");
            printf("~ logging...\n");
            sleep(3);
            printf(FCOLOUR_B_BLUE "");
            dog_display_server_logs(0);
            printf(FCOLOUR_DEFAULT "\n");
          }
        } else {
          printf(FCOLOUR_DEFAULT "\n");
          pr_color(stdout,FCOLOUR_RED,"Server startup failed!\n");
          if(is_pterodactyl_env()) {
            goto server_done;
          }
          elapsed=difftime(end,start);
          if(elapsed<=4.1 && ret_serv==0) {
            ret_serv=1;
            printf("\ttry starting again..");
            access_debugging_file=path_access(wgconfig.dog_toml_logs);
            if(access_debugging_file)
              remove(wgconfig.dog_toml_logs);
            access_debugging_file=path_access(wgconfig.dog_toml_logs);
            if(access_debugging_file)
              remove(wgconfig.dog_toml_logs);
            end=time(NULL);
            goto back_start;
          }
        }
        printf(FCOLOUR_DEFAULT "\n");
server_done:
        end=time(NULL);
        if(sigint_handler==0)
          raise(SIGINT);

        printf("\x1b[32m==> create debugging runner?\x1b[0m\n");
        char *debug_runner=readline("   answer (y/n): ");
        if(debug_runner && (debug_runner[0]=='\0' || strcmp(debug_runner,"Y")==0 || strcmp(debug_runner,"y")==0)) {
          free(ptr_command);
          ptr_command=strdup("debug");
          free(debug_runner);
          dog_free(command);
          command=NULL;
          goto _reexecute_command;
        }
        if(debug_runner) free(debug_runner);
      } else {
        if(dog_compile_running==1) {
          dog_compile_running=0;
          goto start_main;
        }
        dog_exec_samp_server(args2,wgconfig.dog_toml_binary);
        restore_server_config();
        printf("\x1b[32m==> create debugging runner?\x1b[0m\n");
        char *debug_runner=readline("   answer (y/n): ");
        if(debug_runner && (debug_runner[0]=='\0' || strcmp(debug_runner,"Y")==0 || strcmp(debug_runner,"y")==0)) {
          free(ptr_command);
          ptr_command=strdup("debug");
          free(debug_runner);
          dog_free(command);
          command=NULL;
          goto _reexecute_command;
        }
        if(debug_runner) free(debug_runner);
      }
    } else if(dog_server_env()==2) {
      if(args2==NULL || (args2[0]=='.' && args2[1]=='\0')) {
start_main2:
        sa.sa_handler=unit_sigint_handler;
        sigemptyset(&sa.sa_mask);
        sa.sa_flags=SA_RESTART;

        if(sigaction(SIGINT,&sa,NULL)==-1) {
          perror("sigaction");
          exit(EXIT_FAILURE);
        }

        time_t start,end;
        double elapsed;

        int ret_serv=0;
back_start2:
        start=time(NULL);
        printf(FCOLOUR_B_BLUE "");
        #ifdef DOG_WINDOWS
          snprintf(command,DOG_PATH_MAX,"%s",wgconfig.dog_toml_binary);
        #else
          CHMOD_FULL(wgconfig.dog_toml_binary);
          snprintf(command,DOG_PATH_MAX,"./%s",wgconfig.dog_toml_binary);
        #endif
        int rate_runner_failed=dog_exec_command(command);
        if(rate_runner_failed!=0) {
          printf(FCOLOUR_DEFAULT "\n");
          pr_color(stdout,FCOLOUR_RED,"Server startup failed!\n");
          if(is_pterodactyl_env()) {
            goto server_done2;
          }
          elapsed=difftime(end,start);
          if(elapsed<=4.1 && ret_serv==0) {
            ret_serv=1;
            printf("\ttry starting again..");
            access_debugging_file=path_access(wgconfig.dog_toml_logs);
            if(access_debugging_file)
              remove(wgconfig.dog_toml_logs);
            access_debugging_file=path_access(wgconfig.dog_toml_logs);
            if(access_debugging_file)
              remove(wgconfig.dog_toml_logs);
            end=time(NULL);
            goto back_start2;
          }
        }
        printf(FCOLOUR_DEFAULT "\n");
server_done2:
        end=time(NULL);
        if(sigint_handler==0) {
          raise(SIGINT);
        }

        printf("\x1b[32m==> create debugging runner?\x1b[0m\n");
        char *debug_runner=readline("   answer (y/n): ");
        if(debug_runner && (debug_runner[0]=='\0' || strcmp(debug_runner,"Y")==0 || strcmp(debug_runner,"y")==0)) {
          free(ptr_command);
          ptr_command=strdup("debug");
          free(debug_runner);
          dog_free(command);
          command=NULL;
          goto _reexecute_command;
        }
        if(debug_runner) free(debug_runner);
      } else {
        if(dog_compile_running==1) {
          dog_compile_running=0;
          goto start_main2;
        }
        dog_exec_omp_server(args2,wgconfig.dog_ptr_omp);
        restore_server_config();
        printf("\x1b[32m==> create debugging runner?\x1b[0m\n");
        char *debug_runner=readline("   answer (y/n): ");
        if(debug_runner && (debug_runner[0]=='\0' || strcmp(debug_runner,"Y")==0 || strcmp(debug_runner,"y")==0)) {
          free(ptr_command);
          ptr_command=strdup("debug");
          free(debug_runner);
          dog_free(command);
          command=NULL;
          goto _reexecute_command;
        }
        if(debug_runner) free(debug_runner);
      }
    } else {
      pr_error(stdout,
               "\033[1;31merror:\033[0m sa-mp/open.mp server not found!\n"
               "  \033[2mhelp:\033[0m install it before continuing\n");
      printf("\n  \033[1mInstall now?\033[0m  [\033[32mY\033[0m/\033[31mn\033[0m]: ");

      char *pointer_signalA;
ret_ptr3:
      pointer_signalA=readline("");

      while(true) {
        if(pointer_signalA && (strcmp(pointer_signalA,"Y")==0 || strcmp(pointer_signalA,"y")==0)) {
          free(pointer_signalA);
          if(!strcmp(wgconfig.dog_os_type,OS_SIGNAL_WINDOWS)) {
            int ret=dog_install_server("windows");
n_loop_igm:
            if(ret==-1 && wgconfig.dog_sel_stat!=0)
              goto n_loop_igm;
          } else if(!strcmp(wgconfig.dog_os_type,OS_SIGNAL_LINUX)) {
            int ret=dog_install_server("linux");
n_loop_igm2:
            if(ret==-1 && wgconfig.dog_sel_stat!=0)
              goto n_loop_igm2;
          }
          break;
        } else if(pointer_signalA && (strcmp(pointer_signalA,"N")==0 || strcmp(pointer_signalA,"n")==0)) {
          free(pointer_signalA);
          break;
        } else {
          pr_error(stdout,"Invalid input. Please type Y/y to install or N/n to cancel.");
          free(pointer_signalA);
          pointer_signalA=NULL;
          goto ret_ptr3;
        }
      }
    }

    if(command) {
      free(command);
      command=NULL;
    }
    goto unit_done;
  } else if(strncmp(ptr_command,"compiles",strlen("compiles"))==0) {
    dog_console_title("Watchdogs | @ compiles");

    char *args=ptr_command+strlen("compiles");
    while(*args==' ') ++args;
    char *args2=NULL;
    args2=strtok(args," ");

    if(args2==NULL || args2[0]=='\0') {
      const char *argsc[]={NULL,".",NULL,NULL,NULL,NULL,NULL,NULL,NULL};

      dog_compile_running=1;

      dog_exec_compiler(argsc[0],argsc[1],argsc[2],argsc[3],
                      argsc[4],argsc[5],argsc[6],argsc[7],
                      argsc[8]);

      if(wgconfig.dog_compiler_stat<1) {
        goto _runners_;
      }
    } else {
      const char *argsc[]={NULL,args2,NULL,NULL,NULL,NULL,NULL,NULL,NULL};

      dog_compile_running=1;

      dog_exec_compiler(argsc[0],argsc[1],argsc[2],argsc[3],
                      argsc[4],argsc[5],argsc[6],argsc[7],
                      argsc[8]);

      if(wgconfig.dog_compiler_stat<1) {
        size_t cmd_len=strlen(args)+10;
        char *size_command=dog_malloc(cmd_len);
        if(!size_command) goto unit_done;
        snprintf(size_command,cmd_len,"running %s",args);
        free(ptr_command);
        ptr_command=size_command;

        goto _runners_;
      }
    }

    goto unit_done;
  } else if(strcmp(ptr_command,"stop")==0) {
    dog_console_title("Watchdogs | @ stop");

    dog_stop_server_tasks();

    goto unit_done;
  } else if(strcmp(ptr_command,"restart")==0) {
    dog_console_title("Watchdogs | @ restart");

    dog_stop_server_tasks();
    sleep(2);
    goto _runners_;
  } else if(strncmp(ptr_command,"wanion",strlen("wanion"))==0) {
    char *args=ptr_command+strlen("wanion");
    while(*args==' ') ++args;

    char *raw_file=NULL;
    char *procure_args=strtok(args," ");
    while(procure_args) {
      if(strcmp(procure_args,"--file")==0) {
        procure_args=strtok(NULL," ");
        if(procure_args) raw_file=procure_args;
      }
      procure_args=strtok(NULL," ");
    }

    if(*args=='\0') {
      println(stdout,"Usage: wanion [<text>]");
    } else {
      int retry=0;
      size_t rest_api_len=strlen(wgconfig.dog_toml_models_ai)+100;
      char *size_rest_api_perform=dog_malloc(rest_api_len);
      if(!size_rest_api_perform) goto unit_done;

      int is_chatbot_groq_based=0;
      if(strcmp(wgconfig.dog_toml_chatbot_ai,"gemini")==0)
rest_def:
        snprintf(size_rest_api_perform,rest_api_len,
                 "https://generativelanguage.googleapis.com/"
                 "v1beta/models/%s:generateContent",
                 wgconfig.dog_toml_models_ai);
      else if(strcmp(wgconfig.dog_toml_chatbot_ai,"groq")==0) {
        is_chatbot_groq_based=1;
        snprintf(size_rest_api_perform,rest_api_len,
                 "https://api.groq.com/"
                 "openai/v1/chat/completions");
      } else { goto rest_def; }

      #if defined(_DBG_PRINT)
        printf("rest perform: %s\n",size_rest_api_perform);
      #endif
      size_t escaped_len=strlen(args)*2+1;
      size_t json_len=escaped_len+512;

      char *wanion_escaped_argument=dog_malloc(escaped_len);
      char *wanion_json_payload=dog_malloc(json_len);

      if(!wanion_escaped_argument || !wanion_json_payload) {
        dog_free(wanion_escaped_argument);
        dog_free(wanion_json_payload);
        dog_free(size_rest_api_perform);
        goto unit_done;
      }

      dog_escaping_json(wanion_escaped_argument,args,escaped_len);

      if(is_chatbot_groq_based==1) {
        snprintf(wanion_json_payload,json_len,
                 "{\"model\":\"%s\",\"messages\":[{\"role\":\"user\","
                 "\"content\":\"Your name in here Wanion (use it) made from Groq my asking: %s\""
                 "}],\"max_tokens\":1024}",
                 wgconfig.dog_toml_models_ai,
                 wanion_escaped_argument);
      } else {
        snprintf(wanion_json_payload,json_len,
                 "{\"contents\":[{\"role\":\"user\","
                 "\"parts\":[{\"text\":\"Your name in here Wanion (use it) made from Google my asking: %s\"}]}],"
                 "\"generationConfig\": {\"maxOutputTokens\": 1024}}",
                 wanion_escaped_argument);
      }

      struct timespec start={0},end={0};
      double wanion_dur;
      struct buf b={0};
      struct curl_slist *hdr=NULL;
      size_t tokens_len=strlen(wgconfig.dog_toml_key_ai)+30;
      char *size_tokens=dog_malloc(tokens_len);
      if(!size_tokens) {
        dog_free(wanion_escaped_argument);
        dog_free(wanion_json_payload);
        dog_free(size_rest_api_perform);
        goto unit_done;
      }

wanion_retrying:
      #if defined(_DBG_PRINT)
        printf("json payload: %s\n",wanion_json_payload);
      #endif
      clock_gettime(CLOCK_MONOTONIC,&start);
      CURL *h=curl_easy_init();
      if(!h) {
        clock_gettime(CLOCK_MONOTONIC,&end);
        goto wanion_cleanup;
      }

      hdr=curl_slist_append(NULL,"Content-Type: application/json");
      if(is_chatbot_groq_based==1)
        snprintf(size_tokens,tokens_len,"Authorization: Bearer %s",wgconfig.dog_toml_key_ai);
      else
        snprintf(size_tokens,tokens_len,"x-goog-api-key: %s",wgconfig.dog_toml_key_ai);
      hdr=curl_slist_append(hdr,size_tokens);

      curl_easy_setopt(h,CURLOPT_ACCEPT_ENCODING,"gzip");
      curl_easy_setopt(h,CURLOPT_URL,size_rest_api_perform);
      curl_easy_setopt(h,CURLOPT_HTTPHEADER,hdr);
      curl_easy_setopt(h,CURLOPT_TCP_KEEPALIVE,1L);
      curl_easy_setopt(h,CURLOPT_POSTFIELDS,wanion_json_payload);
      curl_easy_setopt(h,CURLOPT_ACCEPT_ENCODING,"gzip, deflate");
      buf_init(&b);
      curl_easy_setopt(h,CURLOPT_WRITEFUNCTION,write_callbacks);
      curl_easy_setopt(h,CURLOPT_WRITEDATA,&b);
      buf_free(&b);
      curl_easy_setopt(h,CURLOPT_TIMEOUT,30L);

      curl_verify_cacert_pem(h);

      CURLcode res=curl_easy_perform(h);
      if(res!=CURLE_OK) {
        fprintf(stderr,"HTTP request failed: %s\n",curl_easy_strerror(res));
        if(retry!=3) {
          ++retry;
          printf("~ try retrying...\n");
          curl_slist_free_all(hdr);
          curl_easy_cleanup(h);
          hdr=NULL;
          h=NULL;
          goto wanion_retrying;
        }
        curl_slist_free_all(hdr);
        curl_easy_cleanup(h);
        clock_gettime(CLOCK_MONOTONIC,&end);
        goto wanion_cleanup;
      }
      long http_code=0;
      curl_easy_getinfo(h,CURLINFO_RESPONSE_CODE,&http_code);
      if(http_code!=200) {
        fprintf(stderr,"API returned HTTP %ld:\n%s\n",http_code,b.data);
        int rate_api_limit=0;
        if(strfind(b.data,"You exceeded your current quota, please check your plan and billing details",true) ||
           strfind(b.data,"Too Many Requests",true))
          ++rate_api_limit;
        if(rate_api_limit)
          printf("~ limit detected!\n");
        else {
          if(retry!=3) {
            ++retry;
            printf("~ try retrying...\n");
            curl_slist_free_all(hdr);
            curl_easy_cleanup(h);
            hdr=NULL;
            h=NULL;
            goto wanion_retrying;
          }
        }
        curl_slist_free_all(hdr);
        curl_easy_cleanup(h);
        clock_gettime(CLOCK_MONOTONIC,&end);
        goto wanion_cleanup;
      }

      cJSON *root=cJSON_Parse(b.data);
      if(!root) {
        fprintf(stderr,"JSON parse error\n");
        clock_gettime(CLOCK_MONOTONIC,&end);
        goto wanion_curl_end;
      }

      cJSON *error=cJSON_GetObjectItem(root,"error");
      if(error) {
        cJSON *message=cJSON_GetObjectItem(error,"message");
        if(message && cJSON_IsString(message)) {
          fprintf(stderr,"API Error: %s\n",message->valuestring);
        }
        clock_gettime(CLOCK_MONOTONIC,&end);
        goto wanion_json_end;
      }

      #if defined(_DBG_PRINT)
        char *response_str=cJSON_Print(root);
        printf("response: %s\n",response_str);
        dog_free(response_str);
        response_str=NULL;
      #endif
      if(is_chatbot_groq_based==1) {
        char *response_text=NULL;
        cJSON *choices=cJSON_GetObjectItem(root,"choices");
        if(choices && cJSON_IsArray(choices) && cJSON_GetArraySize(choices)>0) {
          cJSON *first_choice=cJSON_GetArrayItem(choices,0);
          cJSON *message=cJSON_GetObjectItem(first_choice,"message");
          if(message) {
            cJSON *content=cJSON_GetObjectItem(message,"content");
            if(cJSON_IsString(content)) {
              response_text=content->valuestring;
            }
          }
        }
        if(!response_text) {
          cJSON *data=cJSON_GetObjectItem(root,"data");
          if(data && cJSON_IsArray(data) && cJSON_GetArraySize(data)>0) {
            cJSON *text_item=cJSON_GetArrayItem(data,0);
            cJSON *text=cJSON_GetObjectItem(text_item,"text");
            if(cJSON_IsString(text)) {
              response_text=text->valuestring;
            }
          }
        }
        if(!response_text) {
          cJSON *text=cJSON_GetObjectItem(root,"text");
          if(cJSON_IsString(text)) {
            response_text=text->valuestring;
          }
        }
        if(response_text) {
          const char *p=response_text;
          int in_bold=0;
          int in_italic=0;
          int in_code=0;
          while(*p) {
            if(!in_code && strncmp(p,"```",3)==0) {
                in_code=1;
                printf("\033[38;5;244m");
                p+=3;
                continue;
            } if(in_code && strncmp(p,"```",3)==0) {
                in_code=0;
                printf("\033[0m");
                p+=3;
                continue;
            } if(in_code) {
                putchar(*p++);
                continue;
            } if(strncmp(p,"**",2)==0) {
                in_bold=!in_bold;
                printf(in_bold ? "\033[1m" : "\033[0m");
                p+=2;
                continue;
            } if(*p=='_') {
                in_italic=!in_italic;
                printf(in_italic ? "\033[3m" : "\033[0m");
                p++;
                continue;
            } if(strncmp(p,"~~",2)==0) {
                printf("\033[9m");
                p+=2;
              continue;
            } if(strncmp(p,"==",2)==0) {
                printf("\033[43m");
                p+=2;
                continue;
            }
            if(p==response_text || *(p-1)=='\n') {
              if(*p=='#' || memcmp(p,"##",2)==0 ||
                 memcmp(p,"###",3)==0) {
                printf("\033[1m");
                p++;
                continue;
              }
            }
            putchar(*p++);
            fflush(stdout);
            #ifdef DOG_LINUX
            usleep(25000);
            #else
            ___usleep(25000);
            #endif
          }
          printf("\033[0m");
        } else {
          fprintf(stderr,"No response text found in Groq response\n");
          if(retry!=3) {
            ++retry;
            printf("~ try retrying...\n");
            curl_slist_free_all(hdr);
            curl_easy_cleanup(h);
            hdr=NULL;
            h=NULL;
            goto wanion_retrying;
          }
          #if defined(_DBG_PRINT)
            printf("Raw Groq response: %s\n",b.data);
          #endif
        }
      } else {
        cJSON *candidates=cJSON_GetObjectItem(root,"candidates");
        if(candidates &&
           cJSON_IsArray(candidates) && cJSON_GetArraySize(candidates)>0) {
          cJSON *candidate=cJSON_GetArrayItem(candidates,0);
          cJSON *content=cJSON_GetObjectItem(candidate,"content");
          if(content) {
            cJSON *parts=cJSON_GetObjectItem(content,"parts");
            if(parts && cJSON_IsArray(parts) && cJSON_GetArraySize(parts)>0) {
              cJSON *part=cJSON_GetArrayItem(parts,0);
              cJSON *text=cJSON_GetObjectItem(part,"text");
              if(cJSON_IsString(text) && text->valuestring) {
                const char *p=text->valuestring;
                int in_bold=0;
                int in_italic=0;
                int in_code=0;
                while(*p) {
                  if(!in_code && strncmp(p,"```",3)==0) {
                        in_code=1;
                        printf("\033[38;5;244m");
                        p+=3;
                        continue;
                  } if(in_code && strncmp(p,"```",3)==0) {
                        in_code=0;
                        printf("\033[0m");
                        p+=3;
                        continue;
                  } if(in_code) {
                        putchar(*p++);
                        continue;
                  } if(strncmp(p,"**",2)==0) {
                        in_bold=!in_bold;
                        printf(in_bold ? "\033[1m" : "\033[0m");
                        p+=2;
                        continue;
                  } if(*p=='_') {
                        in_italic=!in_italic;
                        printf(in_italic ? "\033[3m" : "\033[0m");
                        p++;
                        continue;
                  } if(strncmp(p,"~~",2)==0) {
                        printf("\033[9m");
                        p+=2;
                        continue;
                  } if(strncmp(p,"==",2)==0) {
                        printf("\033[43m");
                        p+=2;
                        continue;
                  }
                  if(p==text->valuestring || *(p-1)=='\n') {
                    if(*p=='#' || memcmp(p,"##",2)==0 ||
                       memcmp(p,"###",3)==0) {
                      printf("\033[1m");
                      p++;
                      continue;
                    }
                  }
                  putchar(*p++);
                  fflush(stdout);
                  #ifdef DOG_LINUX
                  usleep(25000);
                  #else
                  ___usleep(25000);
                  #endif
                }
                printf("\033[0m");
              } else fprintf(stderr,"No response text found\n");
            } else {
              fprintf(stderr,"No parts found in content\n");
              if(retry!=3) {
                ++retry;
                printf("~ try retrying...\n");
                curl_slist_free_all(hdr);
                curl_easy_cleanup(h);
                hdr=NULL;
                h=NULL;
                goto wanion_retrying;
              }
            }
          } else fprintf(stderr,"No content found in candidate\n");
        } else {
          fprintf(stderr,"No candidates found in response\n");
        }
      }

      clock_gettime(CLOCK_MONOTONIC,&end);

      wanion_dur=(end.tv_sec-start.tv_sec)
                +(end.tv_nsec-start.tv_nsec)/1e9;

      printf("\n");
      pr_color(stdout,FCOLOUR_CYAN,
               " <W> Finished at %.3fs (%.0f ms)\n",
               wanion_dur,wanion_dur*1000.0);

wanion_json_end:
      cJSON_Delete(root);
wanion_curl_end:
      if(h) {
        curl_slist_free_all(hdr);
        curl_easy_cleanup(h);
      }
      if(b.data) {
        free(b.data);
        b.data=NULL;
      }
wanion_cleanup:
      if(wanion_escaped_argument) {
        free(wanion_escaped_argument);
        wanion_escaped_argument=NULL;
      }
      if(wanion_json_payload) {
        free(wanion_json_payload);
        wanion_json_payload=NULL;
      }
      if(size_tokens) {
        free(size_tokens);
        size_tokens=NULL;
      }
      if(size_rest_api_perform) {
        free(size_rest_api_perform);
        size_rest_api_perform=NULL;
      }
      goto unit_done;
    }
  } else if(strncmp(ptr_command,"tracker",strlen("tracker"))==0) {
    char *args=ptr_command+strlen("tracker");
    while(*args==' ') ++args;

    if(*args=='\0') {
      println(stdout,"Usage: tracker [<name>]");
    } else {
      CURL *curl;
      curl_global_init(CURL_GLOBAL_DEFAULT);
      curl=curl_easy_init();
      if(!curl) {
        fprintf(stderr,"Curl initialization failed!\n");
        goto unit_done;
      }

      int variation_count=0;
      char variations[MAX_VARIATIONS][MAX_USERNAME_LEN];

      tracker_discrepancy(args,variations,&variation_count);

      printf("[TRACKER] Search base: %s\n",args);
      printf("[TRACKER] Generated %d Variations\n\n",variation_count);

      for(int i=0;i<variation_count;i++) {
        printf("=== TRACKING ACCOUNTS: %s ===\n",variations[i]);
        tracking_username(curl,variations[i]);
        printf("\n");
      }

      curl_easy_cleanup(curl);
      curl_global_cleanup();
    }

    goto unit_done;
  } else if(strncmp(ptr_command,"compress",strlen("compress"))==0) {
    char *args=ptr_command+strlen("compress");
    while(*args==' ') args++;

    if(*args=='\0') {
      printf("Usage: compress --file <input> --output <output> --type <format>\n");
      printf("Example:\n\tcompress --file myfile.txt "
             "--output myarchive.zip --type zip\n\t"
             "compress --file myfolder/ "
             "--output myarchive.tar.gz --type gz\n");
      goto unit_done;
    }

    char *raw_input=NULL,*raw_output=NULL,*raw_type=NULL;

    char *procure_args=strtok(args," ");
    while(procure_args) {
      if(strcmp(procure_args,"--file")==0) {
        procure_args=strtok(NULL," ");
        if(procure_args) raw_input=procure_args;
      }
      else if(strcmp(procure_args,"--output")==0) {
        procure_args=strtok(NULL," ");
        if(procure_args) raw_output=procure_args;
      }
      else if(strcmp(procure_args,"--type")==0) {
        procure_args=strtok(NULL," ");
        if(procure_args) raw_type=procure_args;
      }
      procure_args=strtok(NULL," ");
    }

    if(!raw_input || !raw_output || !raw_type) {
      printf("Missing arguments!\n");
      printf("Usage: compress "
             "--file <input> --output <output> --type <zip|tar|gz|bz2|xz>\n");
      printf("Example:\n\tcompress --file myfile.txt "
             "--output myarchive.zip --type zip\n\t"
             "compress --file myfolder/ "
             "--output myarchive.tar.gz --type gz\n");
      goto unit_done;
    }

    CompressionFormat fmt;

    if(strcmp(raw_type,"zip")==0)
      fmt=COMPRESS_ZIP;
    else if(strcmp(raw_type,"tar")==0)
      fmt=COMPRESS_TAR;
    else if(strcmp(raw_type,"gz")==0)
      fmt=COMPRESS_TAR_GZ;
    else if(strcmp(raw_type,"bz2")==0)
      fmt=COMPRESS_TAR_BZ2;
    else if(strcmp(raw_type,"xz")==0)
      fmt=COMPRESS_TAR_XZ;
    else {
      printf("Unknown type: %s\n",raw_type);
      printf("Supported: zip, tar, gz, bz2, xz\n");
      goto unit_done;
    }

    const char *procure_items[]={raw_input};

    int ret=compress_to_archive(raw_output,procure_items,1,fmt);
    if(ret==0)
      pr_info(stdout,"Converter file/folder "
             "to archive (Compression) successfully: %s\n",raw_output);
    else {
      pr_error(stdout,"Compression failed!\n");
      __create_logging();
    }

    goto unit_done;
  } else if(strncmp(ptr_command,"send",strlen("send"))==0) {
    char *args=ptr_command+strlen("send");
    while(*args==' ') ++args;

    if(*args=='\0') {
      println(stdout,"Usage: send [<file_path>]");
    } else {
      if(path_access(args)==0) {
        pr_error(stdout,"file not found: %s",args);
        goto send_done;
      }
      if(!wgconfig.dog_toml_webhooks ||
         strfind(wgconfig.dog_toml_webhooks,"DO_HERE",true) ||
         strlen(wgconfig.dog_toml_webhooks)<1)
      {
        pr_color(stdout,FCOLOUR_YELLOW," ~ Discord webhooks not available");
        goto send_done;
      }

      char *filename=args;
      if(strrchr(args,__PATH_CHR_SEP_LINUX) &&
         strrchr(args,__PATH_CHR_SEP_WIN32))
      {
        filename=(strrchr(args,__PATH_CHR_SEP_LINUX)>strrchr(args,__PATH_CHR_SEP_WIN32))? \
                 strrchr(args,__PATH_CHR_SEP_LINUX)+1:strrchr(args,__PATH_CHR_SEP_WIN32)+1;
      } else if(strrchr(args,__PATH_CHR_SEP_LINUX)) {
        filename=strrchr(args,__PATH_CHR_SEP_LINUX)+1;
      } else if(strrchr(args,__PATH_CHR_SEP_WIN32)) {
        filename=strrchr(args,__PATH_CHR_SEP_WIN32)+1;
      } else {
        ;
      }

      CURL *curl=curl_easy_init();
      if(curl) {
        CURLcode res;
        curl_mime *mime;

        curl_easy_setopt(curl,CURLOPT_SSL_VERIFYPEER,1L);
        curl_easy_setopt(curl,CURLOPT_SSL_VERIFYHOST,2L);
        curl_easy_setopt(curl,CURLOPT_ACCEPT_ENCODING,"gzip");

        mime=curl_mime_init(curl);
        if(!mime) {
          fprintf(stderr,"Failed to create MIME handle\n");
          curl_easy_cleanup(curl);
          goto send_done;
        }

        curl_mimepart *part;

        time_t t=time(NULL);
        struct tm tm=*localtime(&t);
        char *timestamp=dog_malloc(64);
        if(timestamp) {
          snprintf(timestamp,64,"%04d/%02d/%02d %02d:%02d:%02d",
                   tm.tm_year+1900,tm.tm_mon+1,tm.tm_mday,
                   tm.tm_hour,tm.tm_min,tm.tm_sec);
        }

        portable_stat_t st;
        if(portable_stat(filename,&st)==0) {
          char *content_data=dog_malloc(DOG_MAX_PATH);
          if(content_data) {
            snprintf(content_data,DOG_MAX_PATH,
                     "### received send command - %s\n"
                     "> Metadata\n"
                     "- Name: %s\n- Size: %llu bytes\n- Last modified: %llu\n%s",
                     timestamp?timestamp:"unknown",
                     filename,
                     (unsigned long long)st.st_size,
                     (unsigned long long)st.st_lmtime,
                     "-# Please note that if you are using webhooks with a public channel,"
                     "always convert the file into an archive with a password known only to you.");

            part=curl_mime_addpart(mime);
            curl_mime_name(part,"content");
            curl_mime_data(part,content_data,CURL_ZERO_TERMINATED);
            dog_free(content_data);
            content_data=NULL;
          }
        }

        part=curl_mime_addpart(mime);
        curl_mime_name(part,"file");
        curl_mime_filedata(part,args);
        curl_mime_filename(part,filename);

        curl_easy_setopt(curl,CURLOPT_URL,wgconfig.dog_toml_webhooks);
        curl_easy_setopt(curl,CURLOPT_MIMEPOST,mime);

        res=curl_easy_perform(curl);
        if(res!=CURLE_OK)
          fprintf(stderr,"curl_easy_perform() failed: %s\n",curl_easy_strerror(res));

        curl_mime_free(mime);
        curl_easy_cleanup(curl);
        if(timestamp) {
          free(timestamp);
          timestamp=NULL;
        }
      }

      curl_global_cleanup();
    }

send_done:
    goto unit_done;
  } else if(strcmp(ptr_command,"watchdogs")==0||strcmp(ptr_command,"dog")==0) {
    printf("\n  \\/%%#z.       \\/.%%#z./       ,z#%%\\/\n");
    printf("   \\X##k      /X#####X\\      d##X/\n");
    printf("    \\888\\    /888/ \\888\\    /888/\n");
    printf("     `v88;  ;88v'   `v88;  ;88v'\n");
    printf("       \\77xx77/       \\77xx77/\n");
    printf("        `::::'         `::::'\n\n");
    fflush(stdout);
    println(stdout,"Use \"help\" for introduction.");

    if(unit_pre_command && unit_pre_command[0]!='\0')
      goto unit_done;
    else
      goto _ptr_command;
  } else if(strcmp(ptr_command,command_similar)!=0 && dist<=2) {
    dog_console_title("Watchdogs | @ undefined");
    println(stdout, "watchdogs: '%s' is not valid watchdogs command. See 'help'.", ptr_command);
    println(stdout, "   but did you mean '%s'?", command_similar);
    goto unit_done;
  } else {
    int ret;
    size_t cmd_len;
    char *command=NULL;
    ret=-3;
    cmd_len=strlen(ptr_command)+DOG_PATH_MAX;
    command=dog_malloc(cmd_len);
    if(!command) goto unit_done;
    if(is_native_windows()) {
      snprintf(command,cmd_len,
               "powershell -NoLogo -NoProfile -NonInteractive -Command \"%s\"",
               ptr_command);
      goto powershell;
    }
    if(path_access("/bin/sh")!=0)
      snprintf(command,cmd_len,"/bin/sh -c \"%s\"",ptr_command);
    else if(path_access("~/.bashrc")!=0)
      snprintf(command,cmd_len,"bash -c \"%s\"",ptr_command);
    else if(path_access("~/.zshrc")!=0)
      snprintf(command,cmd_len,"zsh -c \"%s\"",ptr_command);
    else
      snprintf(command,cmd_len,"%s",ptr_command);
powershell:
    ret=dog_exec_command(command);
    if(ret)
      dog_console_title("Watchdogs | @ command not found");
    dog_free(command);
    command=NULL;
    if(strcmp(ptr_command,"clear")==0 || strcmp(ptr_command,"cls")==0)
      return -2;
    else
      return -1;
  }

unit_done:
  fflush(stdout);
  if(ptr_command) {
    free(ptr_command);
    ptr_command=NULL;
  }
  if(ptr_prompt) {
    free(ptr_prompt);
    ptr_prompt=NULL;
  }

  return -1;
}

void unit_ret_main(void *unit_pre_command)
{
  dog_console_title(NULL);
  int ret=-3;
  if(unit_pre_command!=NULL) {
    char *procure_command_argv=(char*)unit_pre_command;
    ret=__command__(procure_command_argv);
    clock_gettime(CLOCK_MONOTONIC,&cmd_end);
    if(ret==-2) { return; }
    if(ret==3) { return; }
    command_dur=(cmd_end.tv_sec-cmd_start.tv_sec)+
                (cmd_end.tv_nsec-cmd_start.tv_nsec)/1e9;
    pr_color(stdout,
             FCOLOUR_CYAN,
             " <I> (interactive) Finished at %.3fs\n",
             command_dur);
    return;
  }

loop_main:
  ret=__command__(NULL);
  if(ret==-1) {
    clock_gettime(CLOCK_MONOTONIC,&cmd_end);
    command_dur=(cmd_end.tv_sec-cmd_start.tv_sec)+
                (cmd_end.tv_nsec-cmd_start.tv_nsec)/1e9;
    pr_color(stdout,
             FCOLOUR_CYAN,
             " <I> (interactive) Finished at %.3fs\n",
             command_dur);
    goto loop_main;
  } else if(ret==2) {
    clock_gettime(CLOCK_MONOTONIC,&cmd_end);
    dog_console_title("Terminal.");
    exit(0);
  } else if(ret==-2) {
    clock_gettime(CLOCK_MONOTONIC,&cmd_end);
    goto loop_main;
  } else if(ret==3) {
    clock_gettime(CLOCK_MONOTONIC,&cmd_end);
  } else {
    goto basic_end;
  }

basic_end:

  clock_gettime(CLOCK_MONOTONIC,&cmd_end);
  command_dur=(cmd_end.tv_sec-cmd_start.tv_sec)+
              (cmd_end.tv_nsec-cmd_start.tv_nsec)/1e9;

  pr_color(stdout,
           FCOLOUR_CYAN,
           " <I> (interactive) Finished at %.3fs\n",
           command_dur);
  goto loop_main;
}

int main(int argc,char *argv[])
{
  setvbuf(stdout, NULL, _IONBF, 0);

  __create_unit_logging(0);

  if(argc>1) {
    int i;
    size_t unit_total_len=0;

    for(i=1;i<argc;++i)
      unit_total_len+=strlen(argv[i])+1;

    char *unit_size_prompt=dog_malloc(unit_total_len);
    if(!unit_size_prompt) return 0;

    char *ptr=unit_size_prompt;
    for(i=1;i<argc;++i) {
      if(i>1) *ptr++=' ';
      size_t len=strlen(argv[i]);
      memcpy(ptr,argv[i],len);
      ptr+=len;
    }
    *ptr='\0';

    unit_ret_main(unit_size_prompt);

    dog_free(unit_size_prompt);
    unit_size_prompt=NULL;
    
    return 0;
  } else {
    unit_ret_main(NULL);
  }

  return 0;
}
