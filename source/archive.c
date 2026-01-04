// Copyright (c) 2026 Watchdogs Team and contributors
// All rights reserved. under The 2-Clause BSD License See COPYING or https://opensource.org/license/bsd-2-clause
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>

#include <archive.h>
#include <archive_entry.h>

#include "extra.h"
#include "utils.h"
#include "archive.h"
#include "curl.h"
#include "debug.h"
#include "units.h"

/*  source
    ├── archive.c [x]
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
    ├── units.c
    ├── units.h
    ├── utils.c
    └── utils.h
*/

static void arch_extraction_path(const char *dest,const char *path,
                                 char *out,size_t out_size)
{
  if(strlen(dest)<1 ||
     strcmp(dest,".")==0 ||
     strcmp(dest,"none")==0 ||
     strcmp(dest,"root")==0)
  {
    snprintf(out,out_size,"%s",path);
  }
  else {
    if(strncmp(path,dest,strlen(dest))==0) {
      snprintf(out,out_size,"%s",path);
    }
    else {
      snprintf(out,out_size,"%s" "%s" "%s",
               dest,__PATH_STR_SEP_LINUX,path);
    }
  }
}

static int arch_copy_data(struct archive *ar,struct archive *aw)
{
  size_t size;
  la_int64_t offset;
  int ret=-2;
  const void *buffer;

  while(true) {
    ret=archive_read_data_block(ar,&buffer,&size,&offset);
    if(ret==ARCHIVE_EOF)
      return ARCHIVE_OK;
    if(ret!=ARCHIVE_OK) {
      pr_error(stdout,"arch_copy_data getting error "
               "(read error): %s",archive_error_string(ar));
      return ret;
    }

    ret=archive_write_data_block(aw,buffer,size,offset);
    if(ret!=ARCHIVE_OK) {
      pr_error(stdout,"arch_copy_data getting error "
               "(write error): %s",archive_error_string(aw));
      return ret;
    }
  }
}

int compress_to_archive(const char *archive_path,
                        const char **file_paths,
                        int raw_num_files,
                        CompressionFormat format)
{
  struct archive *archive;
  struct archive_entry *entry;
  struct stat st;
  char buffer[8192];
  int len;
  int fd;
  int ret=0;
  int archive_fd;
  struct stat fd_stat;

  archive=archive_write_new();
  if(archive==NULL) {
    fprintf(stderr,"Failed to create archive object\n");
    return -1;
  }

  switch(format) {
  case COMPRESS_ZIP:
    archive_write_set_format_zip(archive);
    break;

  case COMPRESS_TAR:
    archive_write_set_format_pax_restricted(archive);
    break;

  case COMPRESS_TAR_GZ:
    archive_write_set_format_pax_restricted(archive);
    archive_write_add_filter_gzip(archive);
    break;

  case COMPRESS_TAR_BZ2:
    archive_write_set_format_pax_restricted(archive);
    archive_write_add_filter_bzip2(archive);
    break;

  case COMPRESS_TAR_XZ:
    archive_write_set_format_pax_restricted(archive);
    archive_write_add_filter_xz(archive);
    break;

  default:
    fprintf(stderr,"Unsupported compression format.. default: tgz | zip.\n");
    archive_write_free(archive);
    return -1;
  }

  ret=archive_write_open_filename(archive,archive_path);
  if(ret!=ARCHIVE_OK) {
    fprintf(stderr,"Failed to open archive file %s: %s\n",
            archive_path,archive_error_string(archive));
    archive_write_free(archive);
    return -1;
  }

  for(int i=0;i<raw_num_files;i++) {
    const char *filename=file_paths[i];

    fd=open(filename,O_RDONLY);
    if(fd<0) {
      fprintf(stderr,"Failed to open file %s: %s\n",
              filename,strerror(errno));
      ret=-1;
      continue;
    }

    if(fstat(fd,&fd_stat)!=0) {
      fprintf(stderr,"Failed to fstat file %s: %s\n",
              filename,strerror(errno));
      close(fd);
      ret=-1;
      continue;
    }

    if(!S_ISREG(fd_stat.st_mode)) {
      if(S_ISDIR(fd_stat.st_mode)) {
        close(fd);
        ret=-2;
        goto fallback;
      }
      fprintf(stderr,"File %s is not a regular file\n",filename);
      close(fd);
      ret=-1;
      continue;
    }

    entry=archive_entry_new();
    if(entry==NULL) {
      fprintf(stderr,"Failed to create archive entry for %s\n",filename);
      close(fd);
      ret=-1;
      continue;
    }

    archive_entry_set_pathname(entry,filename);
    archive_entry_set_size(entry,fd_stat.st_size);
    archive_entry_set_filetype(entry,AE_IFREG);
    archive_entry_set_perm(entry,fd_stat.st_mode);
    archive_entry_set_mtime(entry,fd_stat.st_mtime,0);
    archive_entry_set_atime(entry,fd_stat.st_atime,0);
    archive_entry_set_ctime(entry,fd_stat.st_ctime,0);

    ret=archive_write_header(archive,entry);
    if(ret!=ARCHIVE_OK) {
      fprintf(stderr,"Failed to write header for %s: %s\n",
              filename,archive_error_string(archive));
      archive_entry_free(entry);
      close(fd);
      ret=-1;
      continue;
    }

    while((len=read(fd,buffer,sizeof(buffer)))>0) {
      ssize_t written=archive_write_data(archive,buffer,len);
      if(written<0) {
        fprintf(stderr,"Failed to write data for %s: %s\n",
                filename,archive_error_string(archive));
        ret=-1;
        break;
      }
      if(written!=len) {
        fprintf(stderr,"Partial write for %s\n",filename);
        ret=-1;
        break;
      }
    }

    if(len<0) {
      fprintf(stderr,"Failed to read file %s: %s\n",
              filename,strerror(errno));
      ret=-1;
    }

    close(fd);
    archive_entry_free(entry);

    if(ret!=0 && ret!=-2) {
      break;
    }
  }

  archive_write_close(archive);
  archive_write_free(archive);

fallback:
  if(ret==-2) {
    ret=compress_directory(archive_path,file_paths[0],format);
  }

  return ret;
}

int dog_path_recursive(struct archive *archive,const char *root,const char *path)
{
  struct archive_entry *entry=NULL;
  struct stat path_stat;
  char full_path[DOG_MAX_PATH*2];
  int fd=-1;
  struct stat fd_stat;
  ssize_t read_len;
  char buffer[8192];
  DIR *dirp=NULL;
  struct dirent *dent;
  char child_path[DOG_MAX_PATH];

  snprintf(full_path,sizeof(full_path),
           "%s" "%s" "%s",root,__PATH_STR_SEP_LINUX,path);

#ifdef DOG_WINDOWS
  if(stat(full_path,&path_stat)!=0) {
    fprintf(stderr,"stat failed: %s: %s\n",
            full_path,strerror(errno));
    return -1;
  }
#else
  if(lstat(full_path,&path_stat)!=0) {
    fprintf(stderr,"lstat failed: %s: %s\n",
            full_path,strerror(errno));
    return -1;
  }
#endif

  if(S_ISREG(path_stat.st_mode)) {
#ifdef DOG_WINDOWS
    fd=open(full_path,O_RDONLY|O_BINARY);
#else
#ifdef O_NOFOLLOW
#ifdef O_CLOEXEC
    fd=open(full_path,O_RDONLY|O_NOFOLLOW|O_CLOEXEC);
#else
    fd=open(full_path,O_RDONLY|O_NOFOLLOW);
#endif
#else
#ifdef O_CLOEXEC
    fd=open(full_path,O_RDONLY|O_CLOEXEC);
#else
    fd=open(full_path,O_RDONLY);
#endif
#endif
#endif

    if(fd==-1) {
      fprintf(stderr,"open failed: %s: %s\n",full_path,strerror(errno));
      return -1;
    }

    if(fstat(fd,&fd_stat)!=0) {
      fprintf(stderr,"fstat failed: %s: %s\n",full_path,strerror(errno));
      close(fd);
      return -1;
    }

    if(!S_ISREG(fd_stat.st_mode)) {
      fprintf(stderr,"file type changed (not regular): %s\n",full_path);
      close(fd);
      return -1;
    }

    if(path_stat.st_ino!=fd_stat.st_ino || path_stat.st_dev!=fd_stat.st_dev) {
      fprintf(stderr,"file changed during processing: %s\n",full_path);
      close(fd);
      return -1;
    }

    path_stat=fd_stat;

    entry=archive_entry_new();
    if(!entry) {
      fprintf(stderr,"Failed to create archive entry\n");
      close(fd);
      return -1;
    }

    archive_entry_set_pathname(entry,path);
    archive_entry_copy_stat(entry,&path_stat);

    if(archive_write_header(archive,entry)!=ARCHIVE_OK) {
      fprintf(stderr,"archive_write_header failed: %s\n",
              archive_error_string(archive));
      archive_entry_free(entry);
      close(fd);
      return -1;
    }

    while((read_len=read(fd,buffer,sizeof(buffer)))>0) {
      if(archive_write_data(archive,buffer,read_len)<0) {
        fprintf(stderr,"archive_write_data failed: %s\n",
                archive_error_string(archive));
        archive_entry_free(entry);
        close(fd);
        return -1;
      }
    }

    if(read_len<0) {
      fprintf(stderr,"read failed: %s: %s\n",full_path,strerror(errno));
    }

    close(fd);
    archive_entry_free(entry);
    return(read_len<0)?-1:0;
  }

  entry=archive_entry_new();
  if(!entry) {
    fprintf(stderr,"Failed to create archive entry\n");
    return -1;
  }

  archive_entry_set_pathname(entry,path);
  archive_entry_copy_stat(entry,&path_stat);

  if(archive_write_header(archive,entry)!=ARCHIVE_OK) {
    fprintf(stderr,"archive_write_header failed: %s\n",
            archive_error_string(archive));
    archive_entry_free(entry);
    return -1;
  }

  archive_entry_free(entry);

  if(S_ISDIR(path_stat.st_mode)) {
    dirp=opendir(full_path);
    if(!dirp) {
      fprintf(stderr,"opendir failed: %s: %s\n",
              full_path,strerror(errno));
      return -1;
    }

    while((dent=readdir(dirp))!=NULL) {
      if(dog_dot_or_dotdot(dent->d_name))
        continue;

      snprintf(child_path,sizeof(child_path),
               "%s" "%s" "%s",
               path,__PATH_STR_SEP_LINUX,dent->d_name);

      if(dog_path_recursive(archive,root,child_path)!=0) {
        closedir(dirp);
        return -1;
      }
    }

    closedir(dirp);
  }

  return 0;
}

int compress_directory(const char *archive_path,
                       const char *dir_path,
                       CompressionFormat format)
{
  struct archive *a;
  int ret;

  a=archive_write_new();

  if(!a) return -1;

  switch(format) {
  case COMPRESS_ZIP:
    archive_write_set_format_zip(a);
    break;

  case COMPRESS_TAR:
    archive_write_set_format_pax_restricted(a);
    break;

  case COMPRESS_TAR_GZ:
    archive_write_set_format_pax_restricted(a);
    archive_write_add_filter_gzip(a);
    break;

  case COMPRESS_TAR_BZ2:
    archive_write_set_format_pax_restricted(a);
    archive_write_add_filter_bzip2(a);
    break;

  case COMPRESS_TAR_XZ:
    archive_write_set_format_pax_restricted(a);
    archive_write_add_filter_xz(a);
    break;

  default:
    fprintf(stderr,"Unsupported format.. default: tgz | zip.\n");
    archive_write_free(a);
    return -1;
  }

  ret=archive_write_open_filename(a,archive_path);
  if(ret!=ARCHIVE_OK) {
    fprintf(stderr,"Cannot open archive: %s\n",
            archive_error_string(a));
    archive_write_free(a);
    return -1;
  }

  dog_path_recursive(a,dir_path,"");

  archive_write_close(a);
  archive_write_free(a);

  return 0;
}

int dog_extract_tar(const char *tar_path,const char *entry_dest)
{
  int r;
  int flags;
  struct archive *a;
  struct archive *ext;
  struct archive_entry *entry;

  flags=ARCHIVE_EXTRACT_TIME|
        ARCHIVE_EXTRACT_PERM|
        ARCHIVE_EXTRACT_ACL|
        ARCHIVE_EXTRACT_FFLAGS;

  a=archive_read_new();
  archive_read_support_format_all(a);
  archive_read_support_filter_all(a);

  ext=archive_write_disk_new();
  archive_write_disk_set_options(ext,flags);
  archive_write_disk_set_standard_lookup(ext);

  r=archive_read_open_filename(a,tar_path,10240);
  if(r!=ARCHIVE_OK) {
    fprintf(stderr,"Error opening archive: %s\n",archive_error_string(a));
    archive_read_free(a);
    archive_write_free(ext);
    return -1;
  }

  while(true) {
    r=archive_read_next_header(a,&entry);
    if(r==ARCHIVE_EOF) {
      break;
    }
    if(r!=ARCHIVE_OK) {
      fprintf(stderr,"Error reading header: %s\n",archive_error_string(a));
      archive_read_free(a);
      archive_write_free(ext);
      return -1;
    }

    const char *entry_path=NULL;
    entry_path=archive_entry_pathname(entry);

#if defined(_DBG_PRINT)
    printf(" * Extracting: %s\n",entry_path);
    fflush(stdout);
#endif
    if(entry_dest!=NULL && strlen(entry_dest)>0) {
      char entry_new_path[1024];
      dog_mkdir(entry_dest);
      snprintf(entry_new_path,sizeof(entry_new_path),
               "%s" "%s" "%s",entry_dest,__PATH_STR_SEP_LINUX,entry_path);
      archive_entry_set_pathname(entry,entry_new_path);
    }

    r=archive_write_header(ext,entry);
    if(r!=ARCHIVE_OK) {
      fprintf(stderr,"Error writing header for %s: %s\n",
              entry_path,archive_error_string(ext));
    }
    else {
      r=arch_copy_data(a,ext);
      if(r!=ARCHIVE_OK && r!=ARCHIVE_EOF) {
        fprintf(stderr,"Error copying data for %s\n",entry_path);
      }
    }

    r=archive_write_finish_entry(ext);
    if(r!=ARCHIVE_OK) {
      fprintf(stderr,"Error finishing entry %s: %s\n",
              entry_path,archive_error_string(ext));
    }
  }

  archive_read_close(a);
  archive_read_free(a);
  archive_write_close(ext);
  archive_write_free(ext);

  return 0;
}

static int extract_zip_entry(struct archive *archive_read,
                             struct archive *archive_write,
                             struct archive_entry *item)
{
  int ret;
  const void *buffer;
  size_t size;
  la_int64_t offset;

  ret=archive_write_header(archive_write,item);
  if(ret!=ARCHIVE_OK) {
    pr_error(stdout,"Write header error: %s",archive_error_string(archive_write));
    return -1;
  }

  while(true) {
    ret=archive_read_data_block(archive_read,&buffer,&size,&offset);
    if(ret==ARCHIVE_EOF)
      break;
    if(ret<ARCHIVE_OK) {
      pr_error(stdout,"Read data error: %s",archive_error_string(archive_read));
      return -2;
    }

    ret=archive_write_data_block(archive_write,buffer,size,offset);
    if(ret<ARCHIVE_OK) {
      pr_error(stdout,"Write data error: %s",archive_error_string(archive_write));
      return -3;
    }
  }

  return 0;
}

int dog_extract_zip(const char *zip_file,const char *entry_dest)
{
  struct archive *archive_read;
  struct archive *archive_write;
  struct archive_entry *item;
  char paths[DOG_MAX_PATH];
  int ret;
  int error_occurred=0;

  archive_read=archive_read_new();
  archive_write=archive_write_disk_new();

  if(!archive_read || !archive_write) {
    pr_error(stdout,"Failed to create archive handles");
    goto error;
  }

  archive_read_support_format_zip(archive_read);
  archive_read_support_filter_all(archive_read);

  archive_write_disk_set_options(archive_write,ARCHIVE_EXTRACT_TIME);
  archive_write_disk_set_standard_lookup(archive_write);

  ret=archive_read_open_filename(archive_read,zip_file,1024*1024);
  if(ret!=ARCHIVE_OK) {
    pr_error(stdout,"Cannot open archive: %s",
             archive_error_string(archive_read));
    goto error;
  }

  while(archive_read_next_header(archive_read,&item)==ARCHIVE_OK) {
    const char *entry_path;
    entry_path=archive_entry_pathname(item);

#if defined(_DBG_PRINT)
    printf(" * Extracting: %s\n",entry_path);
    fflush(stdout);
#endif
    arch_extraction_path(entry_dest,entry_path,paths,sizeof(paths));
    archive_entry_set_pathname(item,paths);

    if(extract_zip_entry(archive_read,archive_write,item)!=0) {
      error_occurred=1;
      break;
    }
  }

  archive_read_close(archive_read);
  archive_write_close(archive_write);

  archive_read_free(archive_read);
  archive_write_free(archive_write);

  return error_occurred?-1:0;

error:
  if(archive_read)
    archive_read_free(archive_read);
  if(archive_write)
    archive_write_free(archive_write);
  return -1;
}

void dog_extract_archive(const char *filename,const char *dir)
{
  if(dir_exists(".watchdogs")==0)
    MKDIR(".watchdogs");

  pr_color(stdout,FCOLOUR_CYAN,
           " Try Extracting %s archive file...\n",filename);
  fflush(stdout);

  if(strend(filename,".tar.gz",true)) {
    if(dogconfig.dog_idownload==1) {
      if(dir_exists("scripts")!=0) {
        dog_extract_tar(filename,"scripts");
      }
      else {
        MKDIR("scripts");
        dog_extract_tar(filename,"scripts");
      }
    }
    else {
      dog_extract_tar(filename,dir);
    }
  }
  else if(strend(filename,".tar",true)) {
    if(dogconfig.dog_idownload==1) {
      if(dir_exists("scripts")!=0) {
        dog_extract_tar(filename,"scripts");
      }
      else {
        MKDIR("scripts");
        dog_extract_tar(filename,"scripts");
      }
    }
    else {
      dog_extract_tar(filename,dir);
    }
  }
  else if(strend(filename,".zip",true)) {
    if(dogconfig.dog_idownload==1) {
      if(dir_exists("scripts")!=0) {
        dog_extract_zip(filename,"scripts");
      }
      else {
        MKDIR("scripts");
        dog_extract_zip(filename,"scripts");
      }
    }
    else {
      dog_extract_zip(filename,dir);
    }
  }
  else {
    pr_info(stdout,"Undefined archive: %s\n",filename);
  }

  dogconfig.dog_idownload=0;

  return;
}
