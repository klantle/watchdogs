#ifndef ARCHIVE_H
#define ARCHIVE_H

void wg_extract_archive(const char *filename);
int wg_extract_tar(const char *tar_path, const char *dest_path);
int wg_extract_zip(const char *zip_path, const char *dest_path);

#endif
