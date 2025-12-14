#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>

#include <archive.h>
#include <archive_entry.h>  /* libarchive headers for archive manipulation */

#include "extra.h"
#include "utils.h"
#include "archive.h"
#include "curl.h"
#include "debug.h"
#include "units.h"

/*
 * Constructs the full extraction path for an archive entry based on destination directory.
 * Handles edge cases: empty destination, current directory (.), and paths already containing destination.
 * Ensures proper path concatenation without duplicate directory components.
 */
static void arch_extraction_path(const char *dest, const char *path,
                                 char *out, size_t out_size) {

        /* Handle cases where no specific destination is provided */
        if (strlen(dest) < 1 || strcmp(dest, ".") == 0 || strcmp(dest, "none") == 0 || strcmp(dest, "root") == 0) {
                snprintf(out, out_size, "%s", path);
        } else {
                if (!strncmp(path, dest, strlen(dest))) {
                        snprintf(out, out_size,
                                "%s", path);  /* Path already contains destination */
                } else {
                        snprintf(out, out_size,
                                "%s" "%s" "%s",
                                dest, __PATH_STR_SEP_LINUX, path);  /* Combine dest + entry path */
                }
        }
}

/*
 * Copies data blocks from archive reader to archive writer.
 * Handles the actual data transfer between archive read and write streams.
 * Returns ARCHIVE_OK on success, or archive error code on failure.
 */
static int arch_copy_data(struct archive *ar, struct archive *aw)
{
        size_t size;
        la_int64_t offset;
        int ret = -2;
        const void *buffer;

        /* Continuous loop to read and write data blocks until EOF */
        while (true) {
                /* Read a block of data from source archive */
                ret = archive_read_data_block(ar, &buffer, &size, &offset);
                if (ret == ARCHIVE_EOF)
                        return ARCHIVE_OK;  /* Successfully copied all data */
                if (ret != ARCHIVE_OK) {
                        pr_error(stdout, "Read error: %s", archive_error_string(ar));
                        return ret;  /* Return read error */
                }

                /* Write the data block to destination archive */
                ret = archive_write_data_block(aw, buffer, size, offset);
                if (ret != ARCHIVE_OK) {
                        pr_error(stdout, "Write error: %s", archive_error_string(aw));
                        return ret;  /* Return write error */
                }
        }
}

/**
 * Compress files to archive using libarchive
 */
int compress_to_archive(const char *archive_path, 
                const char **file_paths, 
                int raw_num_files,
                CompressionFormat format) {
        
        struct archive *archive;
        struct archive_entry *entry;
        struct stat st;
        char buffer[8192];
        int len;
        int fd;
        int ret = 0;
        int archive_fd;
        struct stat fd_stat;
        
        /* Create archive object */
        archive = archive_write_new();
        if (archive == NULL) {
                fprintf(stderr, "Failed to create archive object\n");
                return -1;
        }
        
        /* Set format based on compression type */
        switch (format) {
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
                fprintf(stderr, "Unsupported compression format\n");
                archive_write_free(archive);
                return -1;
        }
        
        /* Open output archive file */
        ret = archive_write_open_filename(archive, archive_path);
        if (ret != ARCHIVE_OK) {
                fprintf(stderr, "Failed to open archive file %s: %s\n", 
                        archive_path, archive_error_string(archive));
                archive_write_free(archive);
                return -1;
        }
        
        /* Process each input file */
        for (int i = 0; i < raw_num_files; i++) {
                const char *filename = file_paths[i];
                
                /* Open the file first to avoid TOCTOU race condition */
                fd = open(filename, O_RDONLY);
                if (fd < 0) {
                        fprintf(stderr, "Failed to open file %s: %s\n", 
                                filename, strerror(errno));
                        ret = -1;
                        continue;
                }
                
                /* Get file status using the file descriptor for consistency */
                if (fstat(fd, &fd_stat) != 0) {
                        fprintf(stderr, "Failed to fstat file %s: %s\n", 
                                filename, strerror(errno));
                        close(fd);
                        ret = -1;
                        continue;
                }
                
                /* Verify it's a regular file */
                if (!S_ISREG(fd_stat.st_mode)) {
                        /* Make sure it's a directory for fallback */
                        if (S_ISDIR(fd_stat.st_mode)) {
                                close(fd);
                                ret = -2;
                                goto fallback;
                        }
                        fprintf(stderr,
                                "File %s is not a regular file\n", filename);
                        close(fd);
                        ret = -1;
                        continue;
                }
                
                /* Create archive entry for the file */
                entry = archive_entry_new();
                if (entry == NULL) {
                        fprintf(stderr,
                                "Failed to create archive entry for %s\n", filename);
                        close(fd);
                        ret = -1;
                        continue;
                }
                
                /* Set entry properties from file stat */
                archive_entry_set_pathname(entry, filename);
                archive_entry_set_size(entry, fd_stat.st_size);
                archive_entry_set_filetype(entry, AE_IFREG);
                archive_entry_set_perm(entry, fd_stat.st_mode);
                archive_entry_set_mtime(entry, fd_stat.st_mtime, 0);
                archive_entry_set_atime(entry, fd_stat.st_atime, 0);
                archive_entry_set_ctime(entry, fd_stat.st_ctime, 0);
                
                /* Write entry header */
                ret = archive_write_header(archive, entry);
                if (ret != ARCHIVE_OK) {
                        fprintf(stderr, "Failed to write header for %s: %s\n", 
                                filename, archive_error_string(archive));
                        archive_entry_free(entry);
                        close(fd);
                        ret = -1;
                        continue;
                }
                
                /* Write file data to archive */
                while ((len = read(fd, buffer, sizeof(buffer))) > 0) {
                        ssize_t written = archive_write_data(archive, buffer, len);
                        if (written < 0) {
                                fprintf(stderr, "Failed to write data for %s: %s\n", 
                                        filename, archive_error_string(archive));
                                ret = -1;
                                break;
                        }
                        if (written != len) {
                                fprintf(stderr, "Partial write for %s\n", filename);
                                ret = -1;
                                break;
                        }
                }
                
                /* Check for read errors */
                if (len < 0) {
                        fprintf(stderr, "Failed to read file %s: %s\n", 
                                filename, strerror(errno));
                        ret = -1;
                }

                /* Clean up */
                close(fd);
                archive_entry_free(entry);
                
                /* Make sure we should continue after error */
                if (ret != 0 && ret != -2) {
                        break;
                }
        }
        
        /* Close archive */
        archive_write_close(archive);
        archive_write_free(archive);
        
fallback:
        if (ret == -2) {
                /* Fallback to directory compression */
                ret = compress_directory(archive_path, file_paths[0], format);
        }

        return ret;
}

int wg_path_recursive(struct archive *archive, const char *root, const char *path) {

        struct archive_entry *entry = NULL;
        struct stat path_stat;
        char full_path[WG_MAX_PATH * 2];
        int fd = -1;
        struct stat fd_stat;
        ssize_t read_len;
        char buffer[8192];
        DIR *dirp = NULL;
        struct dirent *dent;
        char child_path[WG_MAX_PATH];

        snprintf(full_path, sizeof(full_path),
                "%s" "%s" "%s", root, __PATH_STR_SEP_LINUX, path);

        #ifdef WG_WINDOWS
        if (stat(full_path, &path_stat) != 0) {
                fprintf(stderr,
                        "stat failed: %s: %s\n",
                        full_path, strerror(errno));
                return -1;
        }
        #else
        if (lstat(full_path, &path_stat) != 0) {
                fprintf(stderr,
                        "lstat failed: %s: %s\n",
                        full_path, strerror(errno));
                return -1;
        }
        #endif

        /* Handle regular files with TOCTOU protection */
        if (S_ISREG(path_stat.st_mode)) {
                #ifdef WG_WINDOWS
                fd = open(full_path, O_RDONLY | O_BINARY);
                #else
                #ifdef O_NOFOLLOW
                #ifdef O_CLOEXEC
                fd = open(full_path, O_RDONLY | O_NOFOLLOW | O_CLOEXEC);
                #else
                fd = open(full_path, O_RDONLY | O_NOFOLLOW);
                #endif
                #else
                #ifdef O_CLOEXEC
                fd = open(full_path, O_RDONLY | O_CLOEXEC);
                #else
                fd = open(full_path, O_RDONLY);
                #endif
                #endif
                #endif

                if (fd == -1) {
                        fprintf(stderr,
                                "open failed: %s: %s\n", full_path, strerror(errno));
                        return -1;
                }

                /* CRITICAL: fstat on file descriptor BEFORE any operations */
                if (fstat(fd, &fd_stat) != 0) {
                        fprintf(stderr,
                                "fstat failed: %s: %s\n", full_path, strerror(errno));
                        close(fd);
                        return -1;
                }

                if (!S_ISREG(fd_stat.st_mode)) {
                        fprintf(stderr,
                                "File type changed (not regular): %s\n", full_path);
                        close(fd);
                        return -1;
                }

                if (path_stat.st_ino != fd_stat.st_ino || path_stat.st_dev != fd_stat.st_dev) {
                        fprintf(stderr,
                                "File changed during processing: %s\n", full_path);
                        close(fd);
                        return -1;
                }

                /* Use stat from file descriptor (more reliable) */
                path_stat = fd_stat;

                /* Create archive entry */
                entry = archive_entry_new();
                if (!entry) {
                        fprintf(stderr, "Failed to create archive entry\n");
                        close(fd);
                        return -1;
                }

                archive_entry_set_pathname(entry, path);
                archive_entry_copy_stat(entry, &path_stat);

                if (archive_write_header(archive, entry) != ARCHIVE_OK) {
                        fprintf(stderr,
                                "archive_write_header failed: %s\n", archive_error_string(archive));
                        archive_entry_free(entry);
                        close(fd);
                        return -1;
                }

                /* Write file data to archive */
                while ((read_len = read(fd, buffer, sizeof(buffer))) > 0) {
                        if (archive_write_data(archive, buffer, read_len) < 0) {
                                fprintf(stderr,
                                        "archive_write_data failed: %s\n", archive_error_string(archive));
                                archive_entry_free(entry);
                                close(fd);
                                return -1;
                        }
                }

                if (read_len < 0) {
                        fprintf(stderr,
                                "read failed: %s: %s\n", full_path, strerror(errno));
                }

                close(fd);
                archive_entry_free(entry);
                return (read_len < 0) ? -1 : 0;
        }

        /* Handle directories and other file types (symlinks, devices, etc.) */
        entry = archive_entry_new();
        if (!entry) {
                fprintf(stderr, "Failed to create archive entry\n");
                return -1;
        }
        
        archive_entry_set_pathname(entry, path);
        archive_entry_copy_stat(entry, &path_stat);

        if (archive_write_header(archive, entry) != ARCHIVE_OK) {
                fprintf(stderr,
                        "archive_write_header failed: %s\n",
                        archive_error_string(archive));
                archive_entry_free(entry);
                return -1;
        }

        archive_entry_free(entry);

        /* Recursively process directory contents */
        if (S_ISDIR(path_stat.st_mode)) {

                dirp = opendir(full_path);
                if (!dirp) {
                        fprintf(stderr,
                                "opendir failed: %s: %s\n",
                                full_path, strerror(errno));
                        return -1;
                }

                while ((dent = readdir(dirp)) != NULL) {
                        if (wg_is_special_dir(dent->d_name))
                                continue;

                        snprintf(child_path, sizeof(child_path),
                                "%s" "%s" "%s",
                                path, __PATH_STR_SEP_LINUX, dent->d_name);

                        if ( wg_path_recursive (archive, root, child_path) != 0 ) {
                                closedir(dirp);
                                return -1;
                        }
                }

                closedir(dirp);
        }

        return 0;
}

/**
 * Compresses a directory into an archive file with the specified format.
 */
int compress_directory(const char *archive_path,
                       const char *dir_path,
                       CompressionFormat format) {

        struct archive *a;
        int ret;

        a = archive_write_new();

        if (!a) return -1;

        /* Select archive format */
        switch (format) {
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
                fprintf(stderr, "Unsupported format\n");
                archive_write_free(a);
                return -1;
        }

        /* Open output archive file */
        ret = archive_write_open_filename(a, archive_path);
        if (ret != ARCHIVE_OK) {
                fprintf(stderr, "Cannot open archive: %s\n",
                        archive_error_string(a));
                archive_write_free(a);
                return -1;
        }

        /* Start recursive archiving with empty path as root directory entry */
        wg_path_recursive(a, dir_path, "");

        archive_write_close(a);
        archive_write_free(a);

        return 0;
}

/*
 * Extracts TAR archives (including .tar, .tar.gz, .tar.bz2) using libarchive.
 * Supports full preservation of file metadata (permissions, timestamps, ACLs).
 * Includes interactive logging option for extraction progress monitoring.
 */
int wg_extract_tar(const char *tar_path, const char *entry_dest) {

        int r;
        int flags;
        struct archive *a;          /* Archive reader for reading source archive */
        struct archive *ext;        /* Archive writer for disk extraction */
        struct archive_entry *entry;  /* Current archive entry being processed */

        /* Configure extraction flags to preserve file attributes */
        flags = ARCHIVE_EXTRACT_TIME | ARCHIVE_EXTRACT_PERM |
                        ARCHIVE_EXTRACT_ACL | ARCHIVE_EXTRACT_FFLAGS;

        /* Initialize archive reader with support for all formats and filters */
        a = archive_read_new();
        archive_read_support_format_all(a);
        archive_read_support_filter_all(a);

        /* Initialize archive writer for disk extraction with configured options */
        ext = archive_write_disk_new();
        archive_write_disk_set_options(ext, flags);
        archive_write_disk_set_standard_lookup(ext);

        /* Open the TAR archive file with buffer size 10240 bytes */
        r = archive_read_open_filename(a, tar_path, 10240);
        if (r != ARCHIVE_OK) {
                fprintf(stderr, "Error opening archive: %s\n", archive_error_string(a));
                archive_read_free(a);
                archive_write_free(ext);
                return -1;  /* Return error if archive cannot be opened */
        }

        /* Process each entry in the archive */
        while (true) {
                r = archive_read_next_header(a, &entry);  /* Read next archive header */
                if (r == ARCHIVE_EOF) {
                        break;  /* End of archive reached successfully */
                }
                if (r != ARCHIVE_OK) {
                        fprintf(stderr, "Error reading header: %s\n", archive_error_string(a));
                        archive_read_free(a);
                        archive_write_free(ext);
                        return -1;  /* Return error if header cannot be read */
                }

                const char *entry_path = NULL;
                entry_path = archive_entry_pathname(entry);  /* Get entry's path within archive */

                /* Interactive extraction logging - ask user once if they want extraction logs */
                static int extract_notice = 0;
                static int notice_extracting = 0;

                if (extract_notice == 0) {
                        extract_notice = 1;
                        printf("\x1b[32m==> Create extraction logs?\x1b[0m\n");
                        char *debug_extract = readline("   answer (y/n): ");
                        if (debug_extract) {
                                if (debug_extract[0] == 'Y' || debug_extract[0] == 'y') {
                                        notice_extracting = 1;
                                        printf(" * Extracting: %s\n", entry_path);
                                        fflush(stdout);
                                }
                        }
                        wg_free(debug_extract);
                }
                if (notice_extracting) {
                        printf(" * Extracting: %s\n", entry_path);
                        fflush(stdout);
                }

                /* Modify extraction path if destination directory is specified */
                if (entry_dest != NULL && strlen(entry_dest) > 0) {
                        char entry_new_path[1024];
                        wg_mkdir(entry_dest);  /* Ensure destination directory exists */
                        snprintf(entry_new_path, sizeof(entry_new_path), "%s/%s", entry_dest, entry_path);
                        archive_entry_set_pathname(entry, entry_new_path);  /* Update entry's extraction path */
                }

                /* Write header to disk for current entry */
                r = archive_write_header(ext, entry);
                if (r != ARCHIVE_OK) {
                        fprintf(stderr, "Error writing header for %s: %s\n",
                                        entry_path, archive_error_string(ext));
                } else {
                        /* Copy entry's data from archive to disk */
                        r = arch_copy_data(a, ext);
                        if (r != ARCHIVE_OK && r != ARCHIVE_EOF) {
                                fprintf(stderr, "Error copying data for %s\n", entry_path);
                        }
                }

                /* Finalize the current entry on disk */
                r = archive_write_finish_entry(ext);
                if (r != ARCHIVE_OK) {
                        fprintf(stderr, "Error finishing entry %s: %s\n",
                                        entry_path, archive_error_string(ext));
                }
        }

        /* Cleanup: close and free archive resources */
        archive_read_close(a);
        archive_read_free(a);
        archive_write_close(ext);
        archive_write_free(ext);

        return 0;  /* Success */
}

/*
 * Extracts a single entry from a ZIP archive.
 * Handles header writing and data block copying for ZIP entries.
 * Returns 0 on success, negative values on various types of errors.
 */
static int extract_zip_entry(struct archive *archive_read,
                        struct archive *archive_write,
                        struct archive_entry *item) {

        int ret;
        const void *buffer;
        size_t size;
        la_int64_t offset;

        /* Write entry header to disk */
        ret = archive_write_header(archive_write, item);
        if (ret != ARCHIVE_OK) {
                pr_error(stdout, "Write header error: %s", archive_error_string(archive_write));
                return -1;  /* Header write error */
        }

        /* Copy entry data blocks from archive to disk */
        while (true) {
                ret = archive_read_data_block(archive_read, &buffer, &size, &offset);
                if (ret == ARCHIVE_EOF)
                        break;  /* Successfully copied all data */
                if (ret < ARCHIVE_OK) {
                        pr_error(stdout, "Read data error: %s", archive_error_string(archive_read));
                        return -2;  /* Data read error */
                }

                ret = archive_write_data_block(archive_write, buffer, size, offset);
                if (ret < ARCHIVE_OK) {
                        pr_error(stdout, "Write data error: %s", archive_error_string(archive_write));
                        return -3;  /* Data write error */
                }
        }

        return 0;  /* Success */
}

/*
 * Extracts ZIP archives with support for various ZIP compression formats.
 * Uses libarchive's ZIP format support with proper path handling.
 * Includes interactive extraction logging similar to TAR extraction.
 */
int wg_extract_zip(const char *zip_file, const char *entry_dest) {

        struct archive *archive_read;
        struct archive *archive_write;
        struct archive_entry *item;
        char paths[1024 * 1024];  /* Buffer for full extraction path */
        int ret;
        int error_occurred = 0;  /* Track if any extraction errors occurred */

        /* Initialize archive reader and writer */
        archive_read = archive_read_new();
        archive_write = archive_write_disk_new();

        if (!archive_read || !archive_write) {
                pr_error(stdout, "Failed to create archive handles");
                goto error;  /* Handle allocation failure */
        }

        /* Configure archive reader for ZIP format and all compression filters */
        archive_read_support_format_zip(archive_read);
        archive_read_support_filter_all(archive_read);

        /* Configure archive writer for disk extraction with timestamp preservation */
        archive_write_disk_set_options(archive_write, ARCHIVE_EXTRACT_TIME);
        archive_write_disk_set_standard_lookup(archive_write);

        /* Open the ZIP file with large buffer size for better performance */
        ret = archive_read_open_filename(archive_read, zip_file, 1024 * 1024);
        if (ret != ARCHIVE_OK) {
                pr_error(stdout, "Cannot open file: %s", archive_error_string(archive_read));
                goto error;  /* Handle file open error */
        }

        /* Process each entry in the ZIP archive */
        while (archive_read_next_header(archive_read, &item) == ARCHIVE_OK) {
                const char *entry_path;
                entry_path = archive_entry_pathname(item);  /* Entry's path in archive */

                /* Interactive extraction logging (same as TAR extraction) */
                static int extract_notice = 0;
                static int notice_extracting = 0;

                if (extract_notice == 0) {
                        extract_notice = 1;
                        printf("\x1b[32m==> Create extraction logs?\x1b[0m\n");
                        char *debug_extract = readline("   answer (y/n): ");
                        if (debug_extract) {
                                if (debug_extract[0] == 'Y' || debug_extract[0] == 'y') {
                                        notice_extracting = 1;
                                        printf(" * Extracting: %s\n", entry_path);
                                        fflush(stdout);
                                }
                        }
                        wg_free(debug_extract);
                }
                if (notice_extracting) {
                        printf(" * Extracting: %s\n", entry_path);
                        fflush(stdout);
                }

                /* Construct full extraction path with destination directory */
                arch_extraction_path(entry_dest, entry_path, paths, sizeof(paths));
                archive_entry_set_pathname(item, paths);  /* Update entry's extraction path */

                /* Extract the current entry */
                if (extract_zip_entry(archive_read, archive_write, item) != 0) {
                        error_occurred = 1;  /* Mark error but continue with other entries */
                        break;  /* Or continue depending on error handling policy */
                }
        }

        /* Cleanup: close and free archive resources */
        archive_read_close(archive_read);
        archive_write_close(archive_write);

        archive_read_free(archive_read);
        archive_write_free(archive_write);

        return error_occurred ? -1 : 0;  /* Return success or error status */

error:
        /* Error cleanup: free resources if they were allocated */
        if (archive_read)
                archive_read_free(archive_read);
        if (archive_write)
                archive_write_free(archive_write);
        return -1;  /* Return error status */
}

/*
 * Main archive extraction dispatcher that detects archive type and routes to appropriate extractor.
 * Supports .tar.gz, .tar, and .zip formats. Handles automatic destination directory creation.
 */
void wg_extract_archive(const char *filename, const char *dir) {
        
        if (dir_exists(".watchdogs") == 0)
            MKDIR(".watchdogs");

        pr_color(stdout, FCOLOUR_CYAN, " Try Extracting %s archive file...\n", filename);
        fflush(stdout);

        if (strend(filename, ".tar.gz", true)) {
                if (wgconfig.wg_idownload == 1) {
                        if (path_exists("scripts")) {
                                wg_extract_tar(filename, "scripts");
                        } else {
                                if (wg_mkdir(".watchdogs/scripts")) {
                                        pr_info(stdout, "Extracting into .watchdogs/scripts...");
                                        wg_extract_tar(filename, ".watchdogs/scripts");
                                }
                        }
                } else {
                        wg_extract_tar(filename, dir);
                }
        }
        else if (strend(filename, ".tar", true)) {
                if (wgconfig.wg_idownload == 1) {
                        if (path_exists("scripts")) {
                                wg_extract_tar(filename, "scripts");
                        } else {
                                if (wg_mkdir(".watchdogs/scripts")) {
                                        pr_info(stdout, "Extracting into .watchdogs/scripts...");
                                        wg_extract_tar(filename, ".watchdogs/scripts");
                                }
                        }
                } else {
                        wg_extract_tar(filename, dir);
                }
        }
        else if (strend(filename, ".zip", true)) {
                if (wgconfig.wg_idownload == 1) {
                        if (path_exists("scripts")) {
                                wg_extract_zip(filename, "scripts");
                        } else {
                                if (wg_mkdir(".watchdogs/scripts")) {
                                        pr_info(stdout, "Extracting into .watchdogs/scripts...");
                                        wg_extract_zip(filename, ".watchdogs/scripts");
                                }
                        }
                } else {
                        wg_extract_zip(filename, dir);
                }
        }
        else {
                pr_info(stdout, "Undefined archive: %s\n", filename);
        }

        wgconfig.wg_idownload = 0;

        return;
}