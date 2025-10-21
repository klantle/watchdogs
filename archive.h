#ifndef ARCHIVE_H
#define ARCHIVE_H

int watchdogs_extract_archive(const char *tar_files);
void watchdogs_extract_zip(const char *zip_path, const char *dest_path);

#endif
