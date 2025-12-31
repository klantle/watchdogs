#ifndef ARCHIVE_H
#define ARCHIVE_H

typedef enum {
    COMPRESS_ZIP,
    COMPRESS_TAR,
    COMPRESS_TAR_GZ,
    COMPRESS_TAR_BZ2,
    COMPRESS_TAR_XZ
} CompressionFormat;

int compress_to_archive(const char *archive_path, 
                        const char **file_paths, 
                        int num_files,
                        CompressionFormat format);
int compress_directory(const char *archive_path, 
                       const char *dir_path,
                       CompressionFormat format);
void dog_extract_archive(const char *filename, const char *dir);
int dog_extract_tar(const char *tar_path, const char *dest_path);
int dog_extract_zip(const char *zip_path, const char *dest_path);

#endif
