#ifndef ARCHIVE_H
#define ARCHIVE_H

int wd_extract_tar(const char *tar_files);
int wd_extract_zip(const char *zip_path, const char *dest_path);

#endif
