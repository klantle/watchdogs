#ifndef ARCHIVE_H
#define ARCHIVE_H

void wd_extract_archive(const char *filename);
int wd_extract_tar(const char *tar_path, const char *dest_path);
int wd_extract_zip(const char *zip_path, const char *dest_path);

#endif
