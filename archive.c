#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <archive.h>
#include <archive_entry.h>

#include "color.h"
#include "extra.h"
#include "utils.h"
#include "archive.h"
#include "curl.h"
#include "chain.h"

static int arch_copy_data(struct archive *ar,
                          struct archive *aw) {
        int a_read;
        const void *a_buff;
        size_t size;
        la_int64_t offset;
    
        while (1) {
            a_read = archive_read_data_block(ar,
                                             &a_buff,
                                             &size,
                                             &offset);
            if (a_read == ARCHIVE_EOF)
                    return ARCHIVE_OK;
            if (a_read != ARCHIVE_OK) {
                printf_error("Read error: %s", archive_error_string(ar));
                return a_read;
            }
            a_read = archive_write_data_block(aw, a_buff, size, offset);
            if (a_read != ARCHIVE_OK) {
                printf_error("Write error: %s", archive_error_string(aw));
                return a_read;
            }
        }
}

int wd_Extract_TAR(const char *tar_files) {
        struct archive *archive_write = archive_write_disk_new();
        struct archive *archives = archive_read_new();
        struct archive_entry *entry;

        int a_read;
        
        archive_write_disk_set_options(archive_write,
                ARCHIVE_EXTRACT_TIME |
                ARCHIVE_EXTRACT_PERM |
                ARCHIVE_EXTRACT_ACL |
                ARCHIVE_EXTRACT_FFLAGS |
                ARCHIVE_EXTRACT_UNLINK |
                ARCHIVE_EXTRACT_SECURE_SYMLINKS |
                ARCHIVE_EXTRACT_SECURE_NODOTDOT |
                ARCHIVE_EXTRACT_NO_OVERWRITE_NEWER);
            
        archive_read_support_format_all(archives);
        archive_read_support_filter_all(archives);
        
        a_read = archive_read_open_filename(archives,
                                            tar_files,
                                            1024 * 1024);
        if (a_read != ARCHIVE_OK) {
                printf_error("Can't open file: %s", archive_error_string(archives));
                archive_read_free(archives);
                archive_write_free(archive_write);
                return -RETN;
        }
    
        while (1) {
            a_read = archive_read_next_header(archives, &entry);
            if (a_read == ARCHIVE_EOF)
                    break;
            if (a_read != ARCHIVE_OK) {
                        printf_error("header: %s",
                                archive_error_string(archives));
                        break;
            }
    
            a_read = archive_write_header(archive_write, entry);
            if (a_read != ARCHIVE_OK) {
                        printf_error("header: %s",
                                archive_error_string(archive_write));
                        break;
            }
    
            if (archive_entry_size(entry) > 0) {
                a_read = arch_copy_data(archives, archive_write);
                if (a_read != ARCHIVE_OK) {
                        printf_error("data: %s",
                                archive_error_string(archives));
                        break;
                }
            }
    
            archive_write_finish_entry(archive_write);
        }
    
        archive_read_close(archives);
        archive_read_free(archives);
        archive_write_close(archive_write);
        archive_write_free(archive_write);
    
        return (a_read == ARCHIVE_EOF) ? 0 : -1;
}

void wd_Extract_ZIP(const char *zip_path,
                           const char *dest_path)
{
        struct archive *archives;
        struct archive *archive_write;
        struct archive_entry *entry;

        int a_read;

        archives = archive_read_new();
        archive_read_support_format_zip(archives);
        archive_read_support_filter_all(archives);

        if ((a_read = archive_read_open_filename(archives, zip_path, 1024 * 1024))) {
                printf_error("Can't write/open file %s", archive_error_string(archives));
                __main(0);
        }

        archive_write = archive_write_disk_new();
        archive_write_disk_set_options(archive_write, ARCHIVE_EXTRACT_TIME);
        archive_write_disk_set_standard_lookup(archive_write);

        int __has_error = 0x00;

        while (archive_read_next_header(archives, &entry) == ARCHIVE_OK) {
                const char *__cur_file = archive_entry_pathname(entry);

                char ext_full_path[1024 * 1024];

                if (dest_path == NULL || strcmp(dest_path, ".") == 0 || *dest_path == '\0') {
                        snprintf(ext_full_path, sizeof(ext_full_path), "%s", __cur_file);
                } else {
                        if (strncmp(__cur_file, dest_path, strlen(dest_path)) == 0) {
                                snprintf(ext_full_path, sizeof(ext_full_path), "%s", __cur_file);
                        } else {
                                snprintf(ext_full_path, sizeof(ext_full_path), "%s/%s", dest_path, __cur_file);
                        }
                }

                archive_entry_set_pathname(entry, ext_full_path);

                a_read = archive_write_header(archive_write, entry);
                if (a_read != ARCHIVE_OK) {
                        if (!__has_error) {
                                printf_error("during extraction: %s",
                                        archive_error_string(archive_write));
                                __has_error = 0x01;
                                break;
                        }
                } else {
                    const void *a_buff;
                    size_t size;
                    la_int64_t offset;

                    while (1) {
                        a_read = archive_read_data_block(archives, &a_buff, &size, &offset);
                        if (a_read == ARCHIVE_EOF)
                                break;
                        if (a_read < ARCHIVE_OK) {
                                if (!__has_error) {
                                        printf_error("reading block from archive: %s",
                                                archive_error_string(archives));
                                        __has_error = 0x02;
                                        break;
                                }
                        }
                        a_read = archive_write_data_block(archive_write, a_buff, size, offset);
                        if (a_read < ARCHIVE_OK) {
                                if (!__has_error) {
                                        printf_error("writing block to destination: %s",
                                                archive_error_string(archive_write));
                                        __has_error = 0x03;
                                        break;
                                }
                        }
                    }
                }
        }

        archive_read_close(archives);
        archive_read_free(archives);
        archive_write_close(archive_write);
        archive_write_free(archive_write);
}

