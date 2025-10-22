#ifndef ARCHIVE_H
#define ARCHIVE_H

int watch_extract_archive(const char *tar_files);
void watch_extract_zip(const char *zip_path, const char *dest_path);

#endif
