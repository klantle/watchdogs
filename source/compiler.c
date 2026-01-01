#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <limits.h>
#include <time.h>

#ifdef DOG_LINUX
    #include <fcntl.h>
    #include <sys/types.h>
    #include <sys/wait.h>
#endif

#include "utils.h"
#include "units.h"
#include "extra.h"
#include "library.h"
#include "debug.h"
#include "crypto.h"
#include "cause.h"
#include "compiler.h"

const CompilerOption object_opt[]={
    { __FLAG_DEBUG,     " -d2 ", 5 }, { __FLAG_ASSEMBLER, " -a ",  4 },
    { __FLAG_COMPAT,    " -Z+ ", 5 }, { __FLAG_PROLIX,    " -v2 ", 5 },
    { __FLAG_COMPACT,   " -C+ ", 5 },
    { 0, NULL, 0 }
};

#ifndef DOG_WINDOWS
const char *usr_paths[]={
    "/usr/local/lib","/usr/local/lib32", "/data/data/com.termux/files/usr/lib",
    "/data/data/com.termux/files/usr/local/lib", "/data/data/com.termux/arm64/usr/lib",
    "/data/data/com.termux/arm32/usr/lib", "/data/data/com.termux/amd32/usr/lib",
    "/data/data/com.termux/amd64/usr/lib"
};
#endif

static struct timespec pre_start={__compiler_rate_zero}, post_end={__compiler_rate_zero};
static double timer_rate_compile;

static io_compilers dog_compiler_sys={__compiler_rate_zero};

static
  bool
  compilr_with_debugging=false,
  compiler_debugging=false,has_detailed=false,has_debug=false,
  has_clean=false,has_assembler=false,has_compat=false,
  has_verbose=false,has_compact=false,compiler_retrying=false;
static
  FILE
  *this_proc_file=NULL;
static
  char
  pawn_parse[DOG_PATH_MAX]={__compiler_rate_zero},
  temp[DOG_PATH_MAX] = {__compiler_rate_zero},
  buf_log[DOG_MAX_PATH*4]={__compiler_rate_zero},
  command[DOG_PATH_MAX+258]={__compiler_rate_zero},
  include_aio_path[DOG_PATH_MAX*2]={__compiler_rate_zero},
  this_path_include[DOG_PATH_MAX]={__compiler_rate_zero},
  buf[DOG_MAX_PATH]={__compiler_rate_zero},
  compiler_input[DOG_MAX_PATH+DOG_PATH_MAX]={__compiler_rate_zero},
  compiler_extra_options[DOG_PATH_MAX]={__compiler_rate_zero},
  init_flag_for_search[3]={__compiler_rate_zero},
  compiler_pawncc_path[DOG_PATH_MAX]={__compiler_rate_zero},
  compiler_proj_path[DOG_PATH_MAX]={__compiler_rate_zero};
static
  size_t
  size_init_flag_for_search;
static
  char
  *compiler_size_last_slash=NULL,
  *compiler_back_slash=NULL,
  *size_include_extra=NULL,
  *procure_string_pos=NULL,
  *expect=NULL,
  *pointer_signalA=NULL,
  *platform=NULL,
  *proj_targets=NULL,
  *dog_compiler_unix_args[DOG_MAX_PATH+256]={NULL},
  *compiler_unix_token=NULL;
static
  toml_table_t
  *dog_toml_config=NULL;
static
  char
  dog_buffer_error[DOG_PATH_MAX];
#ifdef DOG_WINDOWS
  static PROCESS_INFORMATION pi;
  static STARTUPINFO         si;
  static SECURITY_ATTRIBUTES sa;
#endif

int dog_exec_compiler(const char *args,const char *compile_args_val, const char *second_arg,const char *four_arg,
const char *five_arg,const char *six_arg, const char *seven_arg,const char *eight_arg, const char *nine_arg)
{
  io_compilers comp; io_compilers *revolver_compiler=&comp;

  const char*
    argv_buf[]={
    second_arg,four_arg, five_arg,six_arg, seven_arg,eight_arg, nine_arg
  };

  if(dir_exists(".watchdogs")==0)
    MKDIR(".watchdogs");

  compilr_with_debugging=false,
  compiler_debugging=false,has_detailed=false,has_debug=false,
  has_clean=false,has_assembler=false,has_compat=false,
  has_verbose=false,has_compact=false,compiler_retrying=false;

  this_proc_file=NULL;

  memset(pawn_parse,__compiler_rate_zero, sizeof(pawn_parse));
  memset(temp,__compiler_rate_zero, sizeof(temp));
  memset(buf_log,__compiler_rate_zero, sizeof(buf_log));
  memset(command,__compiler_rate_zero, sizeof(command));
  memset(include_aio_path,__compiler_rate_zero, sizeof(include_aio_path));
  memset(this_path_include,__compiler_rate_zero, sizeof(this_path_include));
  memset(buf,__compiler_rate_zero, sizeof(buf));
  memset(compiler_input,__compiler_rate_zero, sizeof(compiler_input));
  memset(compiler_extra_options,__compiler_rate_zero, sizeof(compiler_extra_options));
  memset(init_flag_for_search,__compiler_rate_zero, sizeof(init_flag_for_search));
  memset(compiler_pawncc_path,__compiler_rate_zero, sizeof(compiler_pawncc_path));
  memset(compiler_proj_path,__compiler_rate_zero, sizeof(compiler_proj_path));

  size_init_flag_for_search=sizeof(init_flag_for_search);
  compiler_size_last_slash=NULL;
  compiler_back_slash=NULL;
  size_include_extra=NULL;
  procure_string_pos=NULL;
  expect=NULL;
  pointer_signalA=NULL;
  platform=NULL;

  proj_targets=NULL;

  memset(dog_compiler_unix_args,__compiler_rate_zero, sizeof(dog_compiler_unix_args));

  compiler_unix_token=NULL;
  dog_toml_config=NULL;

  compiler_memory_clean();

#ifdef DOG_LINUX
  static int rate_export_path=0;

  if(rate_export_path<1) {
    size_t counts=sizeof(usr_paths)/sizeof(usr_paths[0]);

    char _newpath[DOG_MAX_PATH],_so_path[DOG_PATH_MAX];
    const char *_old=getenv("LD_LIBRARY_PATH");
    if(!_old) _old="";

    snprintf(_newpath,sizeof(_newpath),"%s",_old);

    for(size_t i=0;i<counts;i++) {
      snprintf(_so_path,sizeof(_so_path),"%s/libpawnc.so",usr_paths[i]);
      if(path_exists(_so_path)!=0) {
        if(_newpath[0]!='\0') strncat(_newpath,":",sizeof(_newpath)-strlen(_newpath)-1);
        strncat(_newpath,usr_paths[i],sizeof(_newpath)-strlen(_newpath)-1);
      }
    }
    
    if(_newpath[0]!='\0') {
      setenv("LD_LIBRARY_PATH",_newpath,1);
      pr_info(stdout,"LD_LIBRARY_PATH set to: %s",_newpath);
      ++rate_export_path;
    } else {
      pr_warning(stdout,"libpawnc.so not found in any target path..");
    }
  }
#endif
  
  char *_pointer_pawncc=NULL;
  int __rate_pawncc_exists=0;

  if(strcmp(wgconfig.dog_toml_os_type,OS_SIGNAL_WINDOWS)==0) {
    _pointer_pawncc="pawncc.exe";
  } else if(strcmp(wgconfig.dog_toml_os_type,OS_SIGNAL_LINUX)==0) {
    _pointer_pawncc="pawncc";
  }
  
  if(dir_exists("pawno")!=0 && dir_exists("qawno")!=0) {
    __rate_pawncc_exists=dog_sef_fdir("pawno",_pointer_pawncc,NULL);
    if(__rate_pawncc_exists) {
      ;
    } else {
      __rate_pawncc_exists=dog_sef_fdir("qawno",_pointer_pawncc,NULL);
      if(__rate_pawncc_exists<1) {
        __rate_pawncc_exists=dog_sef_fdir(".",_pointer_pawncc,NULL);
      }
    }
  } else if(dir_exists("pawno")!=0) {
    __rate_pawncc_exists=dog_sef_fdir("pawno",_pointer_pawncc,NULL);
    if(__rate_pawncc_exists) {
      ;
    } else {
      __rate_pawncc_exists=dog_sef_fdir(".",_pointer_pawncc,NULL);
    }
  } else if(dir_exists("qawno")!=0) {
    __rate_pawncc_exists=dog_sef_fdir("qawno",_pointer_pawncc,NULL);
    if(__rate_pawncc_exists) {
      ;
    } else {
      __rate_pawncc_exists=dog_sef_fdir(".",_pointer_pawncc,NULL);
    }
  } else {
    __rate_pawncc_exists=dog_sef_fdir(".",_pointer_pawncc,NULL);
  }

  if(__rate_pawncc_exists) {
    this_proc_file=fopen("watchdogs.toml","r");
    if(!this_proc_file) {
      pr_error(stdout,"Can't read file %s","watchdogs.toml");
      __create_logging();
      goto compiler_end;
    }

    dog_toml_config=toml_parse_file(this_proc_file,dog_buffer_error,sizeof(dog_buffer_error));

    if(this_proc_file) {
      fclose(this_proc_file);
      this_proc_file=NULL;
    }

    if(!dog_toml_config) {
      pr_error(stdout,"failed to parse the watchdogs.toml..: %s",dog_buffer_error);
      __create_logging();
      goto compiler_end;
    }

    if(wgconfig.dog_sef_found_list[0]) {
      snprintf(compiler_pawncc_path,
              sizeof(compiler_pawncc_path),"%s",wgconfig.dog_sef_found_list[0]);
    } else {
      pr_error(stdout,"Compiler path not found");
      goto compiler_end;
    }

    if (path_exists(".watchdogs/compiler_test.log")==1) {
        remove(".watchdogs/compiler_test.log");
    }
    if(path_exists(".watchdogs/compiler_test.log")==0) {
      this_proc_file=fopen(".watchdogs/compiler_test.log","w+");
      if(this_proc_file) {
        fclose(this_proc_file);
        this_proc_file=NULL;
        snprintf(command,sizeof(command), "%s -0000000U > .watchdogs/compiler_test.log 2>&1", compiler_pawncc_path);
        dog_exec_command(command);
      }
    }

    this_proc_file=fopen(".watchdogs/compiler_test.log","r");
    if(!this_proc_file) {
      pr_error(stdout,"Failed to open .watchdogs/compiler_test.log");
      __create_logging();
    }

    for(int i=0;i<__compiler_rate_aio_repo;++i) {
      if(argv_buf[i]!=NULL) {
        if(strfind(argv_buf[i],"--detailed",true) ||
           strfind(argv_buf[i],"--watchdogs",true) ||
           strfind(argv_buf[i],"-w",true))
          has_detailed=true;

        if(strfind(argv_buf[i],"--debug",true) ||
           strfind(argv_buf[i],"-d",true))
          has_debug=true;

        if(strfind(argv_buf[i],"--clean",true) ||
           strfind(argv_buf[i],"-n",true))
          has_clean=true;

        if(strfind(argv_buf[i],"--assembler",true) ||
           strfind(argv_buf[i],"-a",true))
          has_assembler=true;

        if(strfind(argv_buf[i],"--compat",true) ||
           strfind(argv_buf[i],"-c",true))
          has_compat=true;

        if(strfind(argv_buf[i],"--prolix",true) ||
           strfind(argv_buf[i],"-p",true))
          has_verbose=true;

        if(strfind(argv_buf[i],"--compact",true) ||
           strfind(argv_buf[i],"-t",true))
          has_compact=true;
      }
    }

    toml_table_t *dog_compiler=toml_table_in(dog_toml_config,"compiler");
    if(dog_compiler)
    {
      toml_array_t *option_arr=toml_array_in(dog_compiler,"option");
      if(option_arr) {
        expect=NULL;
        size_t toml_array_size;
        toml_array_size=toml_array_nelem(option_arr);

        for(size_t i=0;i<toml_array_size;i++) {
          toml_datum_t toml_option_value;
          toml_option_value=toml_string_at(option_arr,i);
          if(!toml_option_value.ok)
            continue;

          if(strlen(toml_option_value.u.s)>=2) {
            snprintf(init_flag_for_search, size_init_flag_for_search,"%.2s", toml_option_value.u.s);
          } else {
            strncpy(init_flag_for_search,toml_option_value.u.s,size_init_flag_for_search-1);
          }

          if(this_proc_file!=NULL) 
            {
              rewind(this_proc_file);
              while(fgets(buf_log,sizeof(buf_log),this_proc_file)!=NULL &&
                strfind(buf_log,"error while loading shared libraries:",true))
                {
                  dog_printfile(".watchdogs/compiler_test.log");
                  goto compiler_end;
                }
            }

          char *_compiler_options=toml_option_value.u.s;
          while(*_compiler_options && isspace(*_compiler_options))
            ++_compiler_options;

          if(*_compiler_options!='-') {
            pr_color(stdout,FCOLOUR_GREEN,
                "[COMPILER]: "FCOLOUR_CYAN"\"%s\" "FCOLOUR_DEFAULT"is not valid compiler flag!",
                toml_option_value.u.s);
            sleep(2);
            printf("\n");
            dog_printfile(".watchdogs/compiler_test.log");
            dog_free(toml_option_value.u.s);
            goto compiler_end;
          }

          if(strfind(toml_option_value.u.s,"-d",true) || has_debug>0)
            compiler_debugging=true;

          size_t old_len=expect ? strlen(expect):0,new_len=old_len+strlen(toml_option_value.u.s)+2;

          char *tmp=dog_realloc(expect,new_len);
          if(!tmp) {
            dog_free(expect);
            dog_free(toml_option_value.u.s);
            expect=NULL;
            break;
          }

          expect=tmp;

          if(!old_len)
            snprintf(expect,new_len,"%s",toml_option_value.u.s);
          else
            snprintf(expect+old_len, new_len-old_len, " %s",toml_option_value.u.s);

          dog_free(toml_option_value.u.s);
          toml_option_value.u.s=NULL;
        }

        if(this_proc_file) {
          fclose(this_proc_file);
          this_proc_file=NULL;
          if(path_access(".watchdogs/compiler_test.log"))
            remove(".watchdogs/compiler_test.log");
        }

        if(expect) {
          dog_free(wgconfig.dog_toml_aio_opt);
          wgconfig.dog_toml_aio_opt=expect;
          expect=NULL;
        } else {
          dog_free(wgconfig.dog_toml_aio_opt);
          wgconfig.dog_toml_aio_opt=strdup("");
          if(!wgconfig.dog_toml_aio_opt) {
            pr_error(stdout,"Memory allocation failed");
            goto compiler_end;
          }
        }
      }

_compiler_retrying:
      if (compiler_retrying) {
          has_compat=true;
          has_compact=true;
          has_debug=true;
          has_detailed=true;
      }

      {
        unsigned int flags=0;

        if(has_debug)
          flags |= __FLAG_DEBUG;

        if(has_assembler)
          flags |= __FLAG_ASSEMBLER;

        if(has_compat)
          flags |= __FLAG_COMPAT;

        if(has_verbose)
          flags |= __FLAG_PROLIX;

        if(has_compact)
          flags |= __FLAG_COMPACT;

        char *p = compiler_extra_options;
        p += strlen(p);

        int i;
        for (i = 0; object_opt[i].option; i++) {
            if (!(flags & object_opt[i].flag))
                continue;

            memcpy(p, object_opt[i].option, object_opt[i].len);
            p += object_opt[i].len;
        }

        *p = '\0';
      }
      
#if defined(_DBG_PRINT)
      compilr_with_debugging=true;
#endif
      if(has_detailed)
        compilr_with_debugging=true;

      if(strlen(compiler_extra_options)>0) {
        size_t current_aio_opt_len=0;

        if(wgconfig.dog_toml_aio_opt) {
          current_aio_opt_len=strlen(wgconfig.dog_toml_aio_opt);
        } else {
          wgconfig.dog_toml_aio_opt=strdup("");
        }

        size_t extra_len=strlen(compiler_extra_options);
        char *new_ptr=dog_realloc(wgconfig.dog_toml_aio_opt,current_aio_opt_len+extra_len+1);
        
        if(!new_ptr) {
          pr_error(stdout,"Memory allocation failed for extra options");
          goto compiler_end;
        }

        wgconfig.dog_toml_aio_opt=new_ptr;
        strcat(wgconfig.dog_toml_aio_opt,compiler_extra_options);
      }

      toml_array_t *toml_include_path=toml_array_in(dog_compiler,"includes");
      if(toml_include_path) {
        int toml_array_size;
        toml_array_size=toml_array_nelem(toml_include_path);

        for(int i=0;i<toml_array_size;i++) {
          toml_datum_t path_val=toml_string_at(toml_include_path,i);
          if(path_val.ok) {
            char size_path_val[DOG_PATH_MAX+26];
            dog_strip_dot_fns(size_path_val,sizeof(size_path_val),path_val.u.s);
            if(size_path_val[0]=='\0') {
              dog_free(path_val.u.s);
              continue;
            }
            if(i>0) {
              size_t cur=strlen(include_aio_path);
              if(cur<sizeof(include_aio_path)-1) {
                snprintf(include_aio_path+cur, sizeof(include_aio_path)-cur, " ");
              }
            }
            size_t cur=strlen(include_aio_path);
            if(cur<sizeof(include_aio_path)-1) {
              snprintf(include_aio_path+cur,
                sizeof(include_aio_path)-cur, "-i%s ", size_path_val);
            }
            dog_free(path_val.u.s);
          }
        }
      }

      bool rate_parent=false;
      if(strfind(compile_args_val,__PARENT_DIR,true) != false) {
        rate_parent=true;
        size_t w=0;
        size_t j;
        bool rate_parent_dir=false;
        for(j=0;compile_args_val[j]!='\0';) {
          if(!rate_parent_dir && strncmp(&compile_args_val[j],__PARENT_DIR,3)==0) {
            j+=3;
            while(compile_args_val[j]!='\0' &&
              compile_args_val[j]!=' ' &&
              compile_args_val[j]!='"') {
              pawn_parse[w++]=compile_args_val[j++];
            }
            size_t s=0;
            for(size_t v=0;v<w;v++) {
              if(pawn_parse[v]==__PATH_CHR_SEP_LINUX ||
                  pawn_parse[v]==__PATH_CHR_SEP_WIN32)
                {
                    s=v+1;
                }
            }
            if(s>0) {
              w=s;
            }
            rate_parent_dir=true;
            break;
          } else j++;
        }

        if(rate_parent_dir && w>0) {
          memmove(pawn_parse+3,pawn_parse,w+1);
          memcpy(pawn_parse,__PARENT_DIR,3);
          w+=3;
          pawn_parse[w]='\0';
          if(pawn_parse[w-1]!=__PATH_CHR_SEP_LINUX && pawn_parse[w-1]!=__PATH_CHR_SEP_WIN32) strcat(pawn_parse,"/");
        } else {
          strcpy(pawn_parse,__PARENT_DIR);
        }

        strcpy(temp,pawn_parse);

        char *gamemodes_slash = "gamemodes/";
        char *gamemodes_back_slash = "gamemodes\\";

        if(strstr(temp,gamemodes_slash) ||
           strstr(temp,gamemodes_back_slash))
          {
            char *pos=strstr(temp,gamemodes_slash);
            if(!pos) pos=strstr(temp,gamemodes_back_slash);
            if(pos) { *pos='\0'; }
          }

        snprintf(buf,sizeof(buf), "-i%s -i%sgamemodes/ "
                "-i%spawno/include/ -i%sqawno/include/ ",
                temp,temp,temp,temp);

        strncpy(this_path_include, buf, sizeof(this_path_include) - 1);
        this_path_include[sizeof(this_path_include) - 1] = '\0';
      } else
        {
          snprintf(buf,sizeof(buf), "-igamemodes/ -ipawno/include/ -iqawno/include ");
          strncpy(this_path_include, buf, sizeof(this_path_include) - 1);
          this_path_include[sizeof(this_path_include) - 1] = '\0';
        }
    
      static int shown=0;
      if (!shown) {
          printf("\n");
          printf(FCOLOUR_YELLOW "Options:\n" FCOLOUR_DEFAULT);

          printf("  %s-w%s, %s--watchdogs%s   Show detailed output\n",
              FCOLOUR_CYAN, FCOLOUR_DEFAULT,
              FCOLOUR_CYAN, FCOLOUR_DEFAULT);

          printf("  %s-d%s, %s--debug%s       Enable debug level 2\n",
              FCOLOUR_CYAN, FCOLOUR_DEFAULT,
              FCOLOUR_CYAN, FCOLOUR_DEFAULT);

          printf("  %s-p%s, %s--prolix%s      Verbose mode\n",
              FCOLOUR_CYAN, FCOLOUR_DEFAULT,
              FCOLOUR_CYAN, FCOLOUR_DEFAULT);

          printf("  %s-a%s, %s--assembler%s   Generate assembler file\n",
              FCOLOUR_CYAN, FCOLOUR_DEFAULT,
              FCOLOUR_CYAN, FCOLOUR_DEFAULT);

          printf("  %s-t%s, %s--compact%s     Compact encoding\n",
              FCOLOUR_CYAN, FCOLOUR_DEFAULT,
              FCOLOUR_CYAN, FCOLOUR_DEFAULT);

          printf("  %s-c%s, %s--compat%s      Compatibility mode\n",
              FCOLOUR_CYAN, FCOLOUR_DEFAULT,
              FCOLOUR_CYAN, FCOLOUR_DEFAULT);

          printf("  %s-n%s, %s--clean%s       Remove '.amx' after compile\n",
              FCOLOUR_CYAN, FCOLOUR_DEFAULT,
              FCOLOUR_CYAN, FCOLOUR_DEFAULT);

          printf("\n");
          fflush(stdout);
          shown=1;
      }

      if(compile_args_val==NULL) {
        compile_args_val="";
      }
      
      if (!is_pterodactyl_env()) {
          /* localhost detecting
            #include <a_samp>

            #if LOCALHOST==1 || MYSQL_LOCALHOST==1 || SQL_LOCALHOST==1 || LOCAL_SERVER==1
                /// also #if defined LOCALHOST
              ... YES
            #else
              ... NO
            #endif
          */
          /* options https://github.com/gskeleton/watchdogs/blob/main/options.txt
            ... ..
            sym=val  define constant "sym" with value "val"
            sym=     define constant "sym" with value 0
          */
          /* cross check */
          pr_info(stdout,"hello, nice day!.\n   We detected that you are compiling in Localhost status.\n"
                      "   Do you want to target the server for localhost or non-localhost (hosting)?\n"
                      "   1/enter = localhost | 2/rand = hosting server.");printf(FCOLOUR_CYAN ">>>");
          static int __cross=-1; /* one-time check */
          if (__cross==-1) {
              char *cross=readline(" ");
              if (cross) {
                  __cross=(strfind(cross,"1",true)==1) ? 1 : 2;
                  dog_free(cross);
              } else {
                  __cross=1;
              }
          }
          if (__cross==1) {
              snprintf(buf,sizeof(buf), "%s LOCALHOST=1 MYSQL_LOCALHOST=1 SQL_LOCALHOST=1 LOCAL_SERVER=1",
                      wgconfig.dog_toml_aio_opt);
              pr_info(stdout, "Activating: " FCOLOUR_CYAN "LOCALHOST=1 MYSQL_LOCALHOST=1 SQL_LOCALHOST=1 LOCAL_SERVER=1");
          } else {
              snprintf(buf,sizeof(buf), "%s LOCALHOST=0 MYSQL_LOCALHOST=0 SQL_LOCALHOST=0 LOCAL_SERVER=0",
                      wgconfig.dog_toml_aio_opt);
              pr_info(stdout, "Disable: " FCOLOUR_CYAN "LOCALHOST=0 MYSQL_LOCALHOST=0 SQL_LOCALHOST=0 LOCAL_SERVER=0");
          }
          dog_free(wgconfig.dog_toml_aio_opt);
          wgconfig.dog_toml_aio_opt=strdup(buf);
      }

      if(*compile_args_val=='\0' || (compile_args_val[0]=='.' && compile_args_val[1]=='\0')) {
        static int compiler_targets=0;
        if(compiler_targets!=1 && strlen(compile_args_val)<1) {
          pr_color(stdout, FCOLOUR_YELLOW,
         "\033[1m====== COMPILER TARGET ======\033[0m\n");
          printf("   ** This notification appears only once.\n"
                "    * You can set the target using args in the command.\n");
          printf("   * You run the command without any args.\n"
                "   * Do you want to compile for " FCOLOUR_GREEN "%s " FCOLOUR_DEFAULT "(just enter), \n"
                "   * or do you want to compile for something else?\n",wgconfig.dog_toml_proj_input);
          printf(FCOLOUR_CYAN ">>>");
          proj_targets=readline(" ");
          if(proj_targets && strlen(proj_targets)>0) {
            dog_free(wgconfig.dog_toml_proj_input);
            wgconfig.dog_toml_proj_input=strdup(proj_targets);
            if(!wgconfig.dog_toml_proj_input) {
              pr_error(stdout,"Memory allocation failed");
              dog_free(proj_targets);
              goto compiler_end;
            }
          }
          dog_free(proj_targets);
          proj_targets=NULL;
          compiler_targets=1;
        }
#ifdef DOG_WINDOWS
        ZeroMemory(&si, sizeof(si));
        si.cb = sizeof(si);

        ZeroMemory(&sa, sizeof(sa));
        sa.nLength = sizeof(sa);
        sa.bInheritHandle = TRUE;
        sa.lpSecurityDescriptor = NULL;

        ZeroMemory(&pi, sizeof(pi));

        HANDLE hFile=CreateFileA(
                ".watchdogs/compiler.log",
                GENERIC_WRITE,
                FILE_SHARE_READ,
                &sa,
                CREATE_ALWAYS,
                FILE_ATTRIBUTE_NORMAL |
                  FILE_FLAG_SEQUENTIAL_SCAN |
                  FILE_ATTRIBUTE_TEMPORARY,
                NULL
        );

        si.dwFlags=STARTF_USESTDHANDLES|STARTF_USESHOWWINDOW;
        si.wShowWindow=SW_HIDE;

        if(hFile!=INVALID_HANDLE_VALUE) {
          si.hStdOutput=hFile;
          si.hStdError=hFile;
        }
        si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);

        int ret_command=0;
        ret_command=snprintf(compiler_input,sizeof(compiler_input),
                        "%s %s -o%s %s %s %s",
            compiler_pawncc_path,
            wgconfig.dog_toml_proj_input,
            wgconfig.dog_toml_proj_output,
            wgconfig.dog_toml_aio_opt,
            include_aio_path,
            this_path_include);

        if(compilr_with_debugging==true) {
           #ifdef DOG_ANDROID
               println(stdout, "%s", compiler_input);
           #else
               dog_console_title(compiler_input);
           #endif
        }

        char multi_compiler_input[sizeof(compiler_input)];
        memcpy(multi_compiler_input, compiler_input, sizeof(multi_compiler_input));

        if(ret_command>0 && ret_command<(int)sizeof(multi_compiler_input)) {
          BOOL win32_process_success;
          win32_process_success=CreateProcessA(
            NULL,
            multi_compiler_input,
            NULL,
            NULL,
            TRUE,
            CREATE_NO_WINDOW|ABOVE_NORMAL_PRIORITY_CLASS,
            NULL,
            NULL,
            &si,
            &pi
          );
          if (hFile != INVALID_HANDLE_VALUE)
            SetHandleInformation(hFile, HANDLE_FLAG_INHERIT, 0);
          if(win32_process_success==TRUE) {
            SetThreadPriority(
              pi.hThread,
              THREAD_PRIORITY_ABOVE_NORMAL
            );

            DWORD_PTR procMask, sysMask;
            GetProcessAffinityMask(GetCurrentProcess(), &procMask, &sysMask);
            SetProcessAffinityMask(pi.hProcess, procMask & ~1);
            
            clock_gettime(CLOCK_MONOTONIC,&pre_start);
            WaitForSingleObject(pi.hProcess,WIN32_TIMEOUT);
            clock_gettime(CLOCK_MONOTONIC,&post_end);

            DWORD proc_exit_code;
            GetExitCodeProcess(pi.hProcess,&proc_exit_code);

            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);
          } else {
            pr_error(stdout,"CreateProcess failed! (%lu)",GetLastError());
            __create_logging();
          }
        } else {
          pr_error(stdout,"ret_compiler too long!");
          __create_logging();
          goto compiler_end;
        }
        if(hFile!=INVALID_HANDLE_VALUE) {
          CloseHandle(hFile);
        }
#else
        int ret_command=0;
        ret_command=snprintf(compiler_input,sizeof(compiler_input),
            "%s %s %s%s %s %s %s",
            compiler_pawncc_path,
            wgconfig.dog_toml_proj_input,
            "-o",
            wgconfig.dog_toml_proj_output,
            wgconfig.dog_toml_aio_opt,
            include_aio_path,
            this_path_include);
        if(ret_command>(int)sizeof(compiler_input)) {
          pr_error(stdout,"ret_compiler too long!");
          __create_logging();
          goto compiler_end;
        }
        fflush(stdout);
        if(compilr_with_debugging==true) {
           #ifdef DOG_ANDROID
               println(stdout, "%s", compiler_input);
           #else
               dog_console_title(compiler_input);
           #endif
        }
        int i=0;
        compiler_unix_token=strtok(compiler_input," ");
        while(compiler_unix_token!=NULL && i<(DOG_MAX_PATH+255)) {
          dog_compiler_unix_args[i++]=compiler_unix_token;
          compiler_unix_token=strtok(NULL," ");
        }
        dog_compiler_unix_args[i]=NULL;

        posix_spawn_file_actions_t process_file_actions;
        posix_spawn_file_actions_init(&process_file_actions);
        int posix_logging_file=open(".watchdogs/compiler.log",O_WRONLY|O_CREAT|O_TRUNC,0644);
        if(posix_logging_file!=-1) {
          posix_spawn_file_actions_adddup2(&process_file_actions,
                  posix_logging_file,
                  STDOUT_FILENO);
          posix_spawn_file_actions_adddup2(&process_file_actions,
                  posix_logging_file,
                  STDERR_FILENO);
          posix_spawn_file_actions_addclose(&process_file_actions,
                  posix_logging_file);
        }

        posix_spawnattr_t spawn_attr;
        posix_spawnattr_init(&spawn_attr);

        posix_spawnattr_setflags(&spawn_attr,
          POSIX_SPAWN_SETSIGDEF|
          POSIX_SPAWN_SETSIGMASK
        );

        pid_t compiler_process_id;
        int process_spawn_result=posix_spawnp(
          &compiler_process_id,
          dog_compiler_unix_args[0],
          &process_file_actions,
          &spawn_attr,
          dog_compiler_unix_args,
          environ
        );

        posix_spawnattr_destroy(&spawn_attr);
        posix_spawn_file_actions_destroy(&process_file_actions);

        if(process_spawn_result==0) {
          int process_status;
          int process_timeout_occurred=0;
          for(int i=0;i<POSIX_TIMEOUT;i++) {
            int p_result=-1;
            clock_gettime(CLOCK_MONOTONIC,&pre_start);
            p_result=waitpid(compiler_process_id,&process_status,WNOHANG);
            clock_gettime(CLOCK_MONOTONIC,&post_end);
            if(p_result==0)
              usleep(0xC350);
            else if(p_result==compiler_process_id) {
              break;
            } else {
              pr_error(stdout,"waitpid error");
              __create_logging();
              break;
            }
            if(i==POSIX_TIMEOUT-1) {
              kill(compiler_process_id,SIGTERM);
              sleep(2);
              kill(compiler_process_id,SIGKILL);
              pr_error(stdout,
                      "posix_spawn process execution timeout! (%d seconds)",POSIX_TIMEOUT);
              __create_logging();
              waitpid(compiler_process_id,&process_status,0);
              process_timeout_occurred=1;
            }
          }
          if(!process_timeout_occurred) {
            if(WIFEXITED(process_status)) {
              int proc_exit_code=0;
              proc_exit_code=WEXITSTATUS(process_status);
              if(proc_exit_code!=0 && proc_exit_code!=1) {
                pr_error(stdout,
                        "compiler process exited with code (%d)",proc_exit_code);
                __create_logging();
              }
            } else if(WIFSIGNALED(process_status)) {
              pr_error(stdout,
                      "compiler process terminated by signal (%d)",WTERMSIG(process_status));
            }
          }
        } else {
          pr_error(stdout,"posix_spawn failed: %s",strerror(process_spawn_result));
          __create_logging();
        }
#endif
        char size_container_output[DOG_PATH_MAX*2];
        snprintf(size_container_output,
                sizeof(size_container_output),"%s",wgconfig.dog_toml_proj_output);
        if(path_exists(".watchdogs/compiler.log")) {
          printf("\n");
          char *ca=NULL;
          ca=size_container_output;
          bool cb=0;
          if (compiler_debugging)
            cb=1;
          if(has_detailed && has_clean) {
            cause_compiler_expl(".watchdogs/compiler.log",ca,cb);
            if(path_exists(ca)) {
              remove(ca);
            }
            goto compiler_done;
          } else if(has_detailed) {
            cause_compiler_expl(".watchdogs/compiler.log",ca,cb);
            goto compiler_done;
          } else if(has_clean) {
            dog_printfile(".watchdogs/compiler.log");
            if(path_exists(ca)) {
              remove(ca);
            }
            goto compiler_done;
          }

          dog_printfile(".watchdogs/compiler.log");

          char log_line[DOG_MAX_PATH*4];
          this_proc_file=fopen(".watchdogs/compiler.log","r");

          if(this_proc_file!=NULL) {
            while(fgets(log_line,sizeof(log_line),this_proc_file)!=NULL) {
              if(strfind(log_line,"backtrace",true))
                pr_color(stdout,FCOLOUR_CYAN,
                        "~ backtrace detected - "
                        "make sure you are using a newer version of pawncc than the one currently in use.");
            }
            fclose(this_proc_file);
            this_proc_file=NULL;
          }
        }
compiler_done:
        this_proc_file=fopen(".watchdogs/compiler.log","r");
        if(this_proc_file) {
          char compiler_line_buffer[DOG_PATH_MAX];
          int has_err=0;
          while(fgets(compiler_line_buffer,sizeof(compiler_line_buffer),this_proc_file)) {
            if(strstr(compiler_line_buffer,"error")) {
              has_err=1;
              break;
            }
          }
          fclose(this_proc_file);
          this_proc_file=NULL;
          if(has_err) {
            if(size_container_output[0]!='\0' && path_access(size_container_output))
              remove(size_container_output);
            wgconfig.dog_compiler_stat=1;
          } else {
            wgconfig.dog_compiler_stat=0;
          }
        } else {
          pr_error(stdout,"Failed to open .watchdogs/compiler.log");
          __create_logging();
        }

        timer_rate_compile=(post_end.tv_sec-pre_start.tv_sec)
                          +(post_end.tv_nsec-pre_start.tv_nsec)/1e9;

        printf("\n");
        pr_color(stdout,FCOLOUR_CYAN,
                " <C> (compile-time) Finished at %.3fs (%.0f ms)\n",
                timer_rate_compile,timer_rate_compile*1000.0);
        if(timer_rate_compile>20) {
          printf("~ This is taking a while, huh?\n"
                "  Make sure you've cleared all the warnings,\n"
                "  you're using the latest compiler,\n"
                "  and double-check that your logic\n"
                "  and pawn algorithm tweaks in the gamemode scripts line up.\n"
                "  make sure the 'MAX_PLAYERS' is not too large and not too small\n");
          fflush(stdout);
        }
      } else {
        strncpy(revolver_compiler->compiler_size_temp,compile_args_val,sizeof(revolver_compiler->compiler_size_temp)-1);
        revolver_compiler->compiler_size_temp[sizeof(revolver_compiler->compiler_size_temp)-1]='\0';

        compiler_size_last_slash=strrchr(revolver_compiler->compiler_size_temp,__PATH_CHR_SEP_LINUX);
        compiler_back_slash=strrchr(revolver_compiler->compiler_size_temp,__PATH_CHR_SEP_WIN32);

        if(compiler_back_slash && (!compiler_size_last_slash || compiler_back_slash>compiler_size_last_slash))
          compiler_size_last_slash=compiler_back_slash;

        if(compiler_size_last_slash) {
          size_t compiler_dir_len;
          compiler_dir_len=(size_t)(compiler_size_last_slash-revolver_compiler->compiler_size_temp);

          if(compiler_dir_len>=sizeof(revolver_compiler->compiler_direct_path))
            compiler_dir_len=sizeof(revolver_compiler->compiler_direct_path)-1;

          memcpy(revolver_compiler->compiler_direct_path,revolver_compiler->compiler_size_temp,compiler_dir_len);
          revolver_compiler->compiler_direct_path[compiler_dir_len]='\0';

          const char *compiler_filename_start=compiler_size_last_slash+1;
          size_t compiler_filename_len;
          compiler_filename_len=strlen(compiler_filename_start);

          if(compiler_filename_len>=sizeof(revolver_compiler->compiler_size_file_name))
            compiler_filename_len=sizeof(revolver_compiler->compiler_size_file_name)-1;

          memcpy(revolver_compiler->compiler_size_file_name,compiler_filename_start,compiler_filename_len);
          revolver_compiler->compiler_size_file_name[compiler_filename_len]='\0';

          size_t total_needed;
          total_needed=strlen(revolver_compiler->compiler_direct_path)+1+
                      strlen(revolver_compiler->compiler_size_file_name)+1;

          if(total_needed>sizeof(revolver_compiler->compiler_size_input_path)) {
            strncpy(revolver_compiler->compiler_direct_path,"gamemodes",
                    sizeof(revolver_compiler->compiler_direct_path)-1);
            revolver_compiler->compiler_direct_path[sizeof(revolver_compiler->compiler_direct_path)-1]='\0';

            size_t compiler_max_size_file_name;
            compiler_max_size_file_name=sizeof(revolver_compiler->compiler_size_file_name)-1;

            if(compiler_filename_len>compiler_max_size_file_name) {
              memcpy(revolver_compiler->compiler_size_file_name,compiler_filename_start,
                    compiler_max_size_file_name);
              revolver_compiler->compiler_size_file_name[compiler_max_size_file_name]='\0';
            }
          }

          if(snprintf(revolver_compiler->compiler_size_input_path,sizeof(revolver_compiler->compiler_size_input_path),
                      "%s/%s",revolver_compiler->compiler_direct_path,revolver_compiler->compiler_size_file_name)>=
              (int)sizeof(revolver_compiler->compiler_size_input_path)) {
            revolver_compiler->compiler_size_input_path[sizeof(revolver_compiler->compiler_size_input_path)-1]='\0';
          }
        } else {
          strncpy(revolver_compiler->compiler_size_file_name,revolver_compiler->compiler_size_temp,
                  sizeof(revolver_compiler->compiler_size_file_name)-1);
          revolver_compiler->compiler_size_file_name[sizeof(revolver_compiler->compiler_size_file_name)-1]='\0';

          strncpy(revolver_compiler->compiler_direct_path,".",sizeof(revolver_compiler->compiler_direct_path)-1);
          revolver_compiler->compiler_direct_path[sizeof(revolver_compiler->compiler_direct_path)-1]='\0';

          if(snprintf(revolver_compiler->compiler_size_input_path,sizeof(revolver_compiler->compiler_size_input_path),
                      "./%s",revolver_compiler->compiler_size_file_name)>=
              (int)sizeof(revolver_compiler->compiler_size_input_path)) {
            revolver_compiler->compiler_size_input_path[sizeof(revolver_compiler->compiler_size_input_path)-1]='\0';
          }
        }

        int compiler_finding_compile_args=0;
        compiler_finding_compile_args=dog_sef_fdir(revolver_compiler->compiler_direct_path,revolver_compiler->compiler_size_file_name,NULL);

        if(!compiler_finding_compile_args &&
          strcmp(revolver_compiler->compiler_direct_path,"gamemodes")!=0) {
          compiler_finding_compile_args=dog_sef_fdir("gamemodes",
                                      revolver_compiler->compiler_size_file_name,NULL);
          if(compiler_finding_compile_args) {
            strncpy(revolver_compiler->compiler_direct_path,"gamemodes",
                    sizeof(revolver_compiler->compiler_direct_path)-1);
            revolver_compiler->compiler_direct_path[sizeof(revolver_compiler->compiler_direct_path)-1]='\0';

            if(snprintf(revolver_compiler->compiler_size_input_path,sizeof(revolver_compiler->compiler_size_input_path),
                        "gamemodes/%s",revolver_compiler->compiler_size_file_name)>=
                (int)sizeof(revolver_compiler->compiler_size_input_path)) {
              revolver_compiler->compiler_size_input_path[sizeof(revolver_compiler->compiler_size_input_path)-1]='\0';
            }

            if(wgconfig.dog_sef_count>RATE_SEF_EMPTY)
              strncpy(wgconfig.dog_sef_found_list[wgconfig.dog_sef_count-1],
                      revolver_compiler->compiler_size_input_path,MAX_SEF_PATH_SIZE);
          }
        }

        if(!compiler_finding_compile_args && !strcmp(revolver_compiler->compiler_direct_path,".")) {
          compiler_finding_compile_args=dog_sef_fdir("gamemodes",
                                      revolver_compiler->compiler_size_file_name,NULL);
          if(compiler_finding_compile_args) {
            strncpy(revolver_compiler->compiler_direct_path,"gamemodes",
                    sizeof(revolver_compiler->compiler_direct_path)-1);
            revolver_compiler->compiler_direct_path[sizeof(revolver_compiler->compiler_direct_path)-1]='\0';

            if(snprintf(revolver_compiler->compiler_size_input_path,sizeof(revolver_compiler->compiler_size_input_path),
                        "gamemodes/%s",revolver_compiler->compiler_size_file_name)>=
                (int)sizeof(revolver_compiler->compiler_size_input_path)) {
              revolver_compiler->compiler_size_input_path[sizeof(revolver_compiler->compiler_size_input_path)-1]='\0';
            }

            if(wgconfig.dog_sef_count>RATE_SEF_EMPTY)
              strncpy(wgconfig.dog_sef_found_list[wgconfig.dog_sef_count-1],
                      revolver_compiler->compiler_size_input_path,MAX_SEF_PATH_SIZE);
          }
        }

        if(wgconfig.dog_sef_found_list[1]) {
          snprintf(compiler_proj_path,
                  sizeof(compiler_proj_path),"%s",wgconfig.dog_sef_found_list[1]);
        } else {
          pr_error(stdout,"Project path not found");
          goto compiler_end;
        }

        if(compiler_finding_compile_args) {
          char size_sef_path[DOG_PATH_MAX];
          snprintf(size_sef_path,sizeof(size_sef_path),"%s",compiler_proj_path);
          char *extension=strrchr(size_sef_path,'.');
          if(extension)
            *extension='\0';

          snprintf(revolver_compiler->container_output,sizeof(revolver_compiler->container_output),"%s",size_sef_path);

          char size_container_output[DOG_MAX_PATH];
          snprintf(size_container_output,sizeof(size_container_output),"%s.amx",revolver_compiler->container_output);

#ifdef DOG_WINDOWS
          ZeroMemory(&si, sizeof(si));
          si.cb = sizeof(si);

          ZeroMemory(&sa, sizeof(sa));
          sa.nLength = sizeof(sa);
          sa.bInheritHandle = TRUE;
          sa.lpSecurityDescriptor = NULL;

          ZeroMemory(&pi, sizeof(pi));

          HANDLE hFile=CreateFileA(
                  ".watchdogs/compiler.log",
                  GENERIC_WRITE,
                  FILE_SHARE_READ,
                  &sa,
                  CREATE_ALWAYS,
                  FILE_ATTRIBUTE_NORMAL |
                    FILE_FLAG_SEQUENTIAL_SCAN |
                    FILE_ATTRIBUTE_TEMPORARY,
                  NULL
          );

          si.dwFlags=STARTF_USESTDHANDLES|STARTF_USESHOWWINDOW;
          si.wShowWindow=SW_HIDE;

          if(hFile!=INVALID_HANDLE_VALUE) {
            si.hStdOutput=hFile;
            si.hStdError=hFile;
          }
          si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);

          int ret_command=0;
          ret_command=snprintf(compiler_input,sizeof(compiler_input),
                          "%s %s -o%s %s %s %s",
              compiler_pawncc_path,
              compiler_proj_path,
              size_container_output,
              wgconfig.dog_toml_aio_opt,
              include_aio_path,
              this_path_include);

          if(compilr_with_debugging==true) {
             #ifdef DOG_ANDROID
                 println(stdout, "%s", compiler_input);
             #else
                 dog_console_title(compiler_input);
             #endif
          }

          char multi_compiler_input[sizeof(compiler_input)];
          memcpy(multi_compiler_input, compiler_input, sizeof(multi_compiler_input));

          if(ret_command>0 && ret_command<(int)sizeof(multi_compiler_input)) {
            BOOL win32_process_success;
            win32_process_success=CreateProcessA(
              NULL,
              multi_compiler_input,
              NULL,
              NULL,
              TRUE,
              CREATE_NO_WINDOW|ABOVE_NORMAL_PRIORITY_CLASS,
              NULL,
              NULL,
              &si,
              &pi
            );
            if (hFile != INVALID_HANDLE_VALUE)
              SetHandleInformation(hFile, HANDLE_FLAG_INHERIT, 0);
            if(win32_process_success==TRUE) {
              SetThreadPriority(
                pi.hThread,
                THREAD_PRIORITY_ABOVE_NORMAL
              );

              DWORD_PTR procMask, sysMask;
              GetProcessAffinityMask(GetCurrentProcess(), &procMask, &sysMask);
              SetProcessAffinityMask(pi.hProcess, procMask & ~1);
              
              clock_gettime(CLOCK_MONOTONIC,&pre_start);
              WaitForSingleObject(pi.hProcess,WIN32_TIMEOUT);
              clock_gettime(CLOCK_MONOTONIC,&post_end);

              DWORD proc_exit_code;
              GetExitCodeProcess(pi.hProcess,&proc_exit_code);

              CloseHandle(pi.hProcess);
              CloseHandle(pi.hThread);
            } else {
              pr_error(stdout,"CreateProcess failed! (%lu)",GetLastError());
              __create_logging();
            }
          } else {
            pr_error(stdout,"ret_compiler too long!");
            __create_logging();
            goto compiler_end;
          }
          if(hFile!=INVALID_HANDLE_VALUE) {
            CloseHandle(hFile);
          }
#else
          int ret_command=0;
          ret_command=snprintf(compiler_input,sizeof(compiler_input),
              "%s %s %s%s %s %s %s",
              compiler_pawncc_path,
              compiler_proj_path,
              "-o",
              size_container_output,
              wgconfig.dog_toml_aio_opt,
              include_aio_path,
              this_path_include);
          if(ret_command>(int)sizeof(compiler_input)) {
            pr_error(stdout,"ret_compiler too long!");
            __create_logging();
            goto compiler_end;
          }
          fflush(stdout);
          if(compilr_with_debugging==true) {
             #ifdef DOG_ANDROID
                 println(stdout, "%s", compiler_input);
             #else
                 dog_console_title(compiler_input);
             #endif
          }
          int i=0;
          compiler_unix_token=strtok(compiler_input," ");
          while(compiler_unix_token!=NULL && i<(DOG_MAX_PATH+255)) {
            dog_compiler_unix_args[i++]=compiler_unix_token;
            compiler_unix_token=strtok(NULL," ");
          }
          dog_compiler_unix_args[i]=NULL;

          posix_spawn_file_actions_t process_file_actions;
          posix_spawn_file_actions_init(&process_file_actions);
          int posix_logging_file=open(".watchdogs/compiler.log",O_WRONLY|O_CREAT|O_TRUNC,0644);
          if(posix_logging_file!=-1) {
            posix_spawn_file_actions_adddup2(&process_file_actions,
                    posix_logging_file,
                    STDOUT_FILENO);
            posix_spawn_file_actions_adddup2(&process_file_actions,
                    posix_logging_file,
                    STDERR_FILENO);
            posix_spawn_file_actions_addclose(&process_file_actions,
                    posix_logging_file);
          }

          posix_spawnattr_t spawn_attr;
          posix_spawnattr_init(&spawn_attr);

          posix_spawnattr_setflags(&spawn_attr,
                  POSIX_SPAWN_SETSIGDEF|
                  POSIX_SPAWN_SETSIGMASK
          );

          pid_t compiler_process_id;
          int process_spawn_result=posix_spawnp(
            &compiler_process_id,
            dog_compiler_unix_args[0],
            &process_file_actions,
            &spawn_attr,
            dog_compiler_unix_args,
            environ
          );

          posix_spawnattr_destroy(&spawn_attr);
          posix_spawn_file_actions_destroy(&process_file_actions);

          if(process_spawn_result==0) {
            int process_status;
            int process_timeout_occurred=0;
            for(int i=0;i<POSIX_TIMEOUT;i++) {
              int p_result=-1;
              clock_gettime(CLOCK_MONOTONIC,&pre_start);
              p_result=waitpid(compiler_process_id,&process_status,WNOHANG);
              clock_gettime(CLOCK_MONOTONIC,&post_end);
              if(p_result==0)
                usleep(0xC350);
              else if(p_result==compiler_process_id) {
                break;
              } else {
                pr_error(stdout,"waitpid error");
                __create_logging();
                break;
              }
              if(i==POSIX_TIMEOUT-1) {
                kill(compiler_process_id,SIGTERM);
                sleep(2);
                kill(compiler_process_id,SIGKILL);
                pr_error(stdout,
                        "posix_spawn process execution timeout! (%d seconds)",POSIX_TIMEOUT);
                __create_logging();
                waitpid(compiler_process_id,&process_status,0);
                process_timeout_occurred=1;
              }
            }
            if(!process_timeout_occurred) {
              if(WIFEXITED(process_status)) {
                int proc_exit_code=0;
                proc_exit_code=WEXITSTATUS(process_status);
                if(proc_exit_code!=0 && proc_exit_code!=1) {
                  pr_error(stdout,
                          "compiler process exited with code (%d)",proc_exit_code);
                  __create_logging();
                }
              } else if(WIFSIGNALED(process_status)) {
                pr_error(stdout,
                        "compiler process terminated by signal (%d)",WTERMSIG(process_status));
                __create_logging();
              }
            }
          } else {
            pr_error(stdout,"posix_spawn failed: %s",strerror(process_spawn_result));
            __create_logging();
          }
#endif
          if(path_exists(".watchdogs/compiler.log")) {
            printf("\n");
            char *ca=NULL;
            ca=size_container_output;
            bool cb=0;
            if (compiler_debugging)
              cb=1;
            if(has_detailed && has_clean) {
              cause_compiler_expl(".watchdogs/compiler.log",ca,cb);
              if(path_exists(ca)) {
                remove(ca);
              }
              goto compiler_done2;
            } else if(has_detailed) {
              cause_compiler_expl(".watchdogs/compiler.log",ca,cb);
              goto compiler_done2;
            } else if(has_clean) {
              dog_printfile(".watchdogs/compiler.log");
              if(path_exists(ca)) {
                remove(ca);
              }
              goto compiler_done2;
            }

            dog_printfile(".watchdogs/compiler.log");

            char log_line[DOG_MAX_PATH*4];
            this_proc_file=fopen(".watchdogs/compiler.log","r");

            if(this_proc_file!=NULL) {
              while(fgets(log_line,sizeof(log_line),this_proc_file)!=NULL) {
                if(strfind(log_line,"backtrace",true))
                  pr_color(stdout,FCOLOUR_CYAN,
                          "~ backtrace detected - "
                          "make sure you are using a newer version of pawncc than the one currently in use.\n");
              }
              fclose(this_proc_file);
              this_proc_file=NULL;
            }
          }
          
compiler_done2:
          this_proc_file=fopen(".watchdogs/compiler.log","r");
          if(this_proc_file) {
            char compiler_line_buffer[DOG_PATH_MAX];
            int has_err=0;
            while(fgets(compiler_line_buffer,sizeof(compiler_line_buffer),this_proc_file)) {
              if(strstr(compiler_line_buffer,"error")) {
                has_err=1;
                break;
              }
            }
            fclose(this_proc_file);
            this_proc_file=NULL;
            if(has_err) {
              if(size_container_output[0]!='\0' && path_access(size_container_output))
                remove(size_container_output);
              wgconfig.dog_compiler_stat=1;
            } else {
              wgconfig.dog_compiler_stat=0;
            }
          } else {
            pr_error(stdout,"Failed to open .watchdogs/compiler.log");
            __create_logging();
          }

          timer_rate_compile=(post_end.tv_sec-pre_start.tv_sec)
                            +(post_end.tv_nsec-pre_start.tv_nsec)/1e9;

          printf("\n");
          pr_color(stdout,FCOLOUR_CYAN,
                  " <C> (compile-time) Finished at %.3fs (%.0f ms)\n",
                  timer_rate_compile,timer_rate_compile*1000.0);
          if(timer_rate_compile>20) {
            printf("~ This is taking a while, huh?\n"
                  "  Make sure you've cleared all the warnings,\n"
                  "  you're using the latest compiler,\n"
                  "  and double-check that your logic\n"
                  "  and pawn algorithm tweaks in the gamemode scripts line up.\n"
                  "  make sure the 'MAX_PLAYERS' is not too large and not too small\n");
            fflush(stdout);
          }
        } else {
          printf("Cannot locate input: " FCOLOUR_CYAN "%s" FCOLOUR_DEFAULT " - No such file or directory\n",compile_args_val);
          goto compiler_end;
        }
      }

      if (this_proc_file)
        fclose(this_proc_file);
      
      memset(buf_log,__compiler_rate_zero, sizeof(buf_log));

      this_proc_file=fopen(".watchdogs/compiler.log", "rb");
      if(this_proc_file!=NULL && compiler_retrying!=1 && has_compat!=1) 
        {
          rewind(this_proc_file);
          while(fgets(buf_log,sizeof(buf_log),this_proc_file)!=NULL) {
            if (strfind(buf_log,"error",true)!=false)
              {
                pr_info(stdout,"compile exit with failed. retrying?");
                printf(FCOLOUR_CYAN ">>>");
                char *retrying=readline(" ");
                if (retrying) {
                  compiler_retrying=true;
                  dog_free(retrying);
                  goto _compiler_retrying;
                }
                dog_free(retrying);
              }
          }
        }

      if (this_proc_file)
        fclose(this_proc_file);

      if(dog_toml_config) {
        toml_free(dog_toml_config);
        dog_toml_config=NULL;
      }
      goto compiler_end;
    }
  } else {
    pr_error(stdout,
            "\033[1;31merror:\033[0m pawncc (our compiler) not found\n"
            "  \033[2mhelp:\033[0m install it before continuing");
    printf("\n  \033[1mInstall now?\033[0m  [\033[32mY\033[0m/\033[31mn\033[0m]: ");

    pointer_signalA=readline("");

    while(true) {
      if(pointer_signalA && (pointer_signalA[0]=='\0' || strcmp(pointer_signalA,"Y")==0 || strcmp(pointer_signalA,"y")==0)) {
        dog_free(pointer_signalA);
        pointer_signalA=NULL;
ret_ptr:
        println(stdout,"Select platform:");
        println(stdout,"-> [L/l] Linux");
        println(stdout,"-> [W/w] Windows");
        pr_color(stdout,FCOLOUR_YELLOW," ^ work's in WSL/MSYS2\n");
        println(stdout,"-> [T/t] Termux");

        wgconfig.dog_sel_stat=1;

        platform=readline("==> ");

        if(platform && strfind(platform,"L",true)) {
          int ret=dog_install_pawncc("linux");
          dog_free(platform);
          platform=NULL;
loop_ipcc:
          if(ret==-1 && wgconfig.dog_sel_stat!=0)
            goto loop_ipcc;
        } else if(platform && strfind(platform,"W",true)) {
          int ret=dog_install_pawncc("windows");
          dog_free(platform);
          platform=NULL;
loop_ipcc2:
          if(ret==-1 && wgconfig.dog_sel_stat!=0)
            goto loop_ipcc2;
        } else if(platform && strfind(platform,"T",true)) {
          int ret=dog_install_pawncc("termux");
          dog_free(platform);
          platform=NULL;
loop_ipcc3:
          if(ret==-1 && wgconfig.dog_sel_stat!=0)
            goto loop_ipcc3;
        } else if(platform && strfind(platform,"E",true)) {
          dog_free(platform);
          platform=NULL;
          goto loop_end;
        } else {
          if(platform) {
            pr_error(stdout,"Invalid platform selection. Input 'E/e' to exit");
            dog_free(platform);
            platform=NULL;
          }
          goto ret_ptr;
        }
loop_end:
        unit_ret_main(NULL);
      } else if(pointer_signalA && (strcmp(pointer_signalA,"N")==0 || strcmp(pointer_signalA,"n")==0)) {
        dog_free(pointer_signalA);
        pointer_signalA=NULL;
        break;
      } else {
        if(pointer_signalA) {
          pr_error(stdout,"Invalid input. Please type Y/y to install or N/n to cancel.");
          dog_free(pointer_signalA);
          pointer_signalA=NULL;
        }
        goto ret_ptr;
      }
    }
  }

compiler_end:
  dog_free(expect);
  if(dog_toml_config) {
    toml_free(dog_toml_config);
  }
  dog_free(proj_targets);
  if(this_proc_file) {
    fclose(this_proc_file);
  }
  if(path_exists(".watchdogs/compiler.log")!=0) {
    remove(".watchdogs/compiler.log");
  }
  return 1;
}
