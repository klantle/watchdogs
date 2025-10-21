#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#define PATH_SEP "\\"
#else
#define PATH_SEP "/"
#endif

#include <archive.h>
#include <archive_entry.h>

#include "utils.h"
#include "archive.h"
#include "curl.h"
#include "watchdogs.h"

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
                printf("Read error: %s\n", archive_error_string(ar));
                return a_read;
            }
            a_read = archive_write_data_block(aw, a_buff, size, offset);
            if (a_read != ARCHIVE_OK) {
                printf("Write error: %s\n", archive_error_string(aw));
                return a_read;
            }
        }
}

int watchdogs_extract_archive(const char *tar_files) {
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
                printf_error("Can't open file: %s\n", archive_error_string(archives));
                archive_read_free(archives);
                archive_write_free(archive_write);
                return -1;
        }
    
        while (1) {
            a_read = archive_read_next_header(archives, &entry);
            if (a_read == ARCHIVE_EOF)
            	break;
            if (a_read != ARCHIVE_OK) {
                        printf_error("header: %s\n",
                                archive_error_string(archives));
                        break;
            }
    
            a_read = archive_write_header(archive_write, entry);
            if (a_read != ARCHIVE_OK) {
                        printf_error("header: %s\n",
                                archive_error_string(archive_write));
                        break;
            }
    
            if (archive_entry_size(entry) > 0) {
                a_read = arch_copy_data(archives, archive_write);
                if (a_read != ARCHIVE_OK) {
                        printf_error("data: %s\n",
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

void watchdogs_extract_zip(const char *zip_path,
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
                printf("Can't resume. sys can't write/open file %s\n", archive_error_string(archives));
                __init(0);
        }

        archive_write = archive_write_disk_new();
        archive_write_disk_set_options(archive_write, ARCHIVE_EXTRACT_TIME);
        archive_write_disk_set_standard_lookup(archive_write);

        int has_error = 0x0;

        while (archive_read_next_header(archives, &entry) == ARCHIVE_OK) {
                const char *__cur_file = archive_entry_pathname(entry);

                char ext_full_path[1024 * 1024];
                snprintf(ext_full_path, sizeof(ext_full_path), "%s/%s",
                        dest_path, __cur_file);
                archive_entry_set_pathname(entry, ext_full_path);

                a_read = archive_write_header(archive_write, entry);
                if (a_read != ARCHIVE_OK) {
                        if (!has_error) {
                                printf_error("during extraction: %s\n",
                                        archive_error_string(archive_write));
                                has_error = 0x1;
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
                                if (!has_error) {
                                        printf_error("reading block from archive: %s\n",
                                                archive_error_string(archives));
                                        has_error = 0x2;
                                        break;
                                }
                        }
                        a_read = archive_write_data_block(archive_write, a_buff, size, offset);
                        if (a_read < ARCHIVE_OK) {
                                if (!has_error) {
                                        printf_error("writing block to destination: %s\n",
                                                archive_error_string(archive_write));
                                        has_error = 0x3;
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

