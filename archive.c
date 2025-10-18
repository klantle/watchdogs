/*
 * What is this?:
 * ------------------------------------------------------------
 * This script is a utility for extracting compressed archive files, 
 * including `.tar`, `.tar.gz`, `.tar.xz`, and `.zip` formats. 
 * It leverages the libarchive library to safely read archive contents 
 * and extract them to a specified destination folder while preserving 
 * file permissions, timestamps, and ensuring symbolic link security.
 *
 * It is part of the "watchdogs" system, which appears to manage or automate 
 * file deployments or updates by extracting downloaded archive files.
 *
 *
 * Script Algorithm:
 * ------------------------------------------------------------
 * 1. Determine the platform (Windows or UNIX) to select the correct path separator.
 * 2. Open the archive file using libarchive APIs (`archive_read_*`).
 * 3. Iterate through all entries (files/folders) in the archive:
 *      - Read the entry header.
 *      - Prepare the extraction path on disk.
 *      - Write the entry header to the destination.
 *      - Copy the entry's data in blocks from the archive to disk.
 *      - Apply attributes such as permissions, timestamps, ACLs, etc.
 * 4. Handle errors at each step and ensure resources (memory, archive handles) are freed.
 *
 *
 * Script Logic:
 * ------------------------------------------------------------
 * Functions:
 *
 * > `arch_copy_data(struct archive *ar, struct archive *aw)`  
 *    - Internal helper function to copy file data from a source archive to disk.
 *    - Reads data blocks using `archive_read_data_block()` and writes them with `archive_write_data_block()`.
 *    - Loops until the end of the entry is reached.
 *
 * > `watchdogs_extract_archive(const char *tar_files)`  
 *    - Extracts tar-based archives (`.tar`, `.tar.gz`, `.tar.xz`, etc.).
 *    - Opens the archive and prepares disk writing options (permissions, timestamps, security).
 *    - Iterates over each entry, copying data via `arch_copy_data`.
 *    - Closes and frees all archive resources.
 *
 * > `watchdogs_extract_zip(const char *zip_path, const char *__dest_path)`  
 *    - Handles extraction of ZIP archives.
 *    - Opens the ZIP archive and configures disk writing options.
 *    - Iterates each entry:
 *        - Builds the full extraction path.
 *        - Writes the header to disk.
 *        - Reads/writes data blocks in a loop.
 *        - Handles and logs any read/write errors.
 *    - Closes and frees all archive resources after extraction.
 *
 *
 * How to Use?:
 * ------------------------------------------------------------
 * 1. Include this source file in your project build and link with libarchive (`-larchive`).
 * 2. Call the extraction functions:
 *      - `watchdogs_extract_archive("path/to/archive.tar.gz");`
 *      - `watchdogs_extract_zip("path/to/archive.zip", "destination/folder");`
 * 3. Ensure the destination folder exists and is writable.
 * 4. On success, the files/folders will be extracted into the destination path.
 * 5. On failure, errors will be printed via `printf_error()`.
 *
 * Notes:
 * - Supports various archive formats automatically via libarchive.
 * - Preserves file attributes and handles symbolic links securely.
 * - Error handling ensures partial extractions do not leave resources open.
 */
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

static int arch_copy_data(struct archive *ar, struct archive *aw) {
        int a_read;
        const void *a_buff;
        size_t size;
        la_int64_t offset;
    
        while (1) {
            a_read = archive_read_data_block(ar, &a_buff, &size, &offset);
            if (a_read == ARCHIVE_EOF) return ARCHIVE_OK;
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
        
        a_read = archive_read_open_filename(archives, tar_files, 1024 * 1024);
        if (a_read != ARCHIVE_OK) {
                printf_error("Can't open file: %s\n", archive_error_string(archives));
                archive_read_free(archives);
                archive_write_free(archive_write);
                return -1;
        }
    
        while (1) {
            a_read = archive_read_next_header(archives, &entry);
            if (a_read == ARCHIVE_EOF) break;
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

void watchdogs_extract_zip(const char *zip_path, const char *__dest_path)
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

                char ext_full_path[128];
                snprintf(ext_full_path, sizeof(ext_full_path), "%s/%s",
                        __dest_path, __cur_file);
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

