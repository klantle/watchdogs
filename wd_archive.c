#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <archive.h>
#include <archive_entry.h>
#include <readline/readline.h>
#include <readline/history.h>

#include "wd_extra.h"
#include "wd_util.h"
#include "wd_archive.h"
#include "wd_curl.h"
#include "wd_unit.h"

/**
 * Copies data blocks from source archive to destination archive
 * Handles the actual data transfer for archive entries with content
 *
 * @param ar Source archive to read data from
 * @param aw Destination archive to write data to
 * @return ARCHIVE_OK on success, error code on failure
 */
static int arch_copy_data(struct archive *ar, struct archive *aw)
{
		int ret;
		const void *buffer;  /* Pointer to the data buffer */
		size_t size;         /* Size of the data buffer */
		la_int64_t offset;   /* Offset within the file */

		/* Continue reading and writing data blocks until end of file */
		while (1) {
				/* Read a block of data from the source archive */
				ret = archive_read_data_block(ar, &buffer, &size, &offset);
				/* Check for end of file condition */
				if (ret == ARCHIVE_EOF)
						return ARCHIVE_OK;
				/* Handle read errors */
				if (ret != ARCHIVE_OK) {
						pr_error(stdout, "Read error: %s", archive_error_string(ar));
						return ret;
				}

				/* Write the data block to the destination archive */
				ret = archive_write_data_block(aw, buffer, size, offset);
				/* Handle write errors */
				if (ret != ARCHIVE_OK) {
						pr_error(stdout, "Write error: %s", archive_error_string(aw));
						return ret;
				}
		}
}

/**
 * Extract tar archive to destination directory
 * @param tar_path Path to the tar archive
 * @param entry_dest Destination directory path (NULL for current directory)
 * @return WD_RETZ on success, -WD_RETN on failure
 */
int wd_extract_tar(const char *tar_path, const char *entry_dest) {
	    struct archive *a;           /* Archive reader object */
	    struct archive *ext;         /* Archive writer/disk extractor object */
	    struct archive_entry *entry; /* Represents a single file/directory in archive */
	    int flags;                  /* Extraction flags */
	    int r;                      /* Return code for archive operations */

	    /**
	     * SET EXTRACTION FLAGS
	     * Configure what metadata to preserve during extraction
	     * ARCHIVE_EXTRACT_TIME - Preserve file timestamps
	     * ARCHIVE_EXTRACT_PERM - Preserve file permissions
	     * ARCHIVE_EXTRACT_ACL - Preserve Access Control Lists
	     * ARCHIVE_EXTRACT_FFLAGS - Preserve file flags
	     */
	    flags = ARCHIVE_EXTRACT_TIME;
	    flags |= ARCHIVE_EXTRACT_PERM;
	    flags |= ARCHIVE_EXTRACT_ACL;
	    flags |= ARCHIVE_EXTRACT_FFLAGS;

	    /**
	     * INITIALIZE ARCHIVE READER
	     * Create and configure the archive reading object
	     * archive_read_new - Create new archive reader
	     * archive_read_support_format_all - Enable all format support
	     * archive_read_support_filter_all - Enable all compression filters
	     */
	    a = archive_read_new();
	    archive_read_support_format_all(a);
	    archive_read_support_filter_all(a);

	    /**
	     * INITIALIZE DISK WRITER
	     * Create and configure the disk extraction object
	     * archive_write_disk_new - Create new disk writer
	     * archive_write_disk_set_options - Apply extraction flags
	     * archive_write_disk_set_standard_lookup - Use system ID lookup
	     */
	    ext = archive_write_disk_new();
	    archive_write_disk_set_options(ext, flags);
	    archive_write_disk_set_standard_lookup(ext);

	    /**
	     * OPEN ARCHIVE FILE
	     * Open the tar file for reading with 10KB buffer
	     * archive_read_open_filename - Open archive file
	     * Returns ARCHIVE_OK on success
	     */
	    r = archive_read_open_filename(a, tar_path, 10240);
	    if (r != ARCHIVE_OK) {
	        fprintf(stderr, "Error opening archive: %s\n", archive_error_string(a));
	        archive_read_free(a);
	        archive_write_free(ext);
	        return -WD_RETN;  /* Return failure */
	    }

	    /**
	     * PROCESS ARCHIVE ENTRIES
	     * Iterate through all files/directories in the archive
	     * archive_read_next_header - Read next file/directory entry
	     * ARCHIVE_EOF indicates successful end of archive
	     */
	    while (1) {
	        r = archive_read_next_header(a, &entry);
	        if (r == ARCHIVE_EOF) {
	            break;  /* End of archive reached successfully */
	        }
	        if (r != ARCHIVE_OK) {
	            fprintf(stderr, "Error reading header: %s\n", archive_error_string(a));
	            archive_read_free(a);
	            archive_write_free(ext);
	            return -WD_RETN;  /* Return failure */
	        }

	        /* Get original path of current entry */
	        const char *entry_path = archive_entry_pathname(entry);

			/* Debugging Notice */
			static int extract_notice = 0;
			static int always_extract_notice = 0;
			if (extract_notice == 0) {
				extract_notice = 1;
				pr_color(stdout, FCOLOUR_GREEN, "* create debugging extracting archive?");
				char *debug_extract = readline(" [y/n]: ");
				if (debug_extract) {
					if (debug_extract[0] == 'Y' || debug_extract[0] == 'y') {
						always_extract_notice = 1;
		    			printf(" * Extracting: %s\n", entry_path);
					}
				}
			}
			if (always_extract_notice) {
				printf(" * Extracting: %s\n", entry_path);
			}

	        /**
	         * MODIFY EXTRACTION PATH IF DESTINATION SPECIFIED
	         * Prepend destination directory to extraction path
	         * wd_mkdir - Should create destination directory recursively
	         * snprintf - Construct new path: entry_dest/original_filename
	         * archive_entry_set_pathname - Update extraction path
	         */
	        if (entry_dest != NULL && strlen(entry_dest) > 0) {
	            char entry_new_path[1024];  /* Buffer for new path */
	            wd_mkdir(entry_dest);  /* Create destination directory */
	            snprintf(entry_new_path, sizeof(entry_new_path), "%s/%s", entry_dest, entry_path);
	            archive_entry_set_pathname(entry, entry_new_path);
	        }

	        /**
	         * WRITE FILE HEADER TO DISK
	         * Creates directory structure and file metadata
	         * archive_write_header - Write entry header to disk
	         */
	        r = archive_write_header(ext, entry);
	        if (r != ARCHIVE_OK) {
	            fprintf(stderr, "Error writing header for %s: %s\n",
	                    entry_path, archive_error_string(ext));
	        } else {
	            /**
	             * COPY FILE DATA
	             * Extract actual file contents from archive to disk
	             * arch_copy_data - Copy file data from archive to disk
	             * Should be archive_read_data_into_archive_write or similar
	             */
	            r = arch_copy_data(a, ext);
	            if (r != ARCHIVE_OK && r != ARCHIVE_EOF) {
	                fprintf(stderr, "Error copying data for %s\n", entry_path);
	            }
	        }

	        /**
	         * FINALIZE ENTRY EXTRACTION
	         * Close file, set permissions, timestamps, etc.
	         * archive_write_finish_entry - Complete entry extraction
	         */
	        r = archive_write_finish_entry(ext);
	        if (r != ARCHIVE_OK) {
	            fprintf(stderr, "Error finishing entry %s: %s\n",
	                    entry_path, archive_error_string(ext));
	        }
	    }

	    /**
	     * CLEANUP RESOURCES
	     * Close and free all archive objects
	     * archive_read_close - Close archive reader
	     * archive_read_free - Free reader memory
	     * archive_write_close - Close disk writer
	     * archive_write_free - Free writer memory
	     */
	    archive_read_close(a);
	    archive_read_free(a);
	    archive_write_close(ext);
	    archive_write_free(ext);

	    return WD_RETZ;  /* Return success */
}

/**
 * Builds the full extraction path for an archive entry
 * Combines destination directory with entry path, handling edge cases
 *
 * @param entry_dest Destination directory path (can be NULL or ".")
 * @param entry_path Path of the entry within the archive
 * @param out_path Output buffer for the full path
 * @param out_size Size of the output buffer
 */
static void build_extraction_path(const char *entry_dest, const char *entry_path,
								  char *out_path, size_t out_size)
{
		/* Handle case where no destination path is specified */
		if (!entry_dest || !strcmp(entry_dest, ".")|| *entry_dest == '\0') {
				/* Extract directly to current directory using entry path */
				wd_snprintf(out_path, out_size, "%s", entry_path);
		} else {
				/* Check if entry path already contains destination path */
				if (!strncmp(entry_path, entry_dest, strlen(entry_dest))) {
						/* Use entry path as-is if it already starts with dest path */
						wd_snprintf(out_path, out_size, "%s", entry_path);
				} else {
						/* Combine destination path with entry path */
						wd_snprintf(out_path, out_size, "%s/%s", entry_dest, entry_path);
				}
		}
}

/**
 * Extracts a single ZIP archive entry to disk
 * Handles both file content and directory creation
 *
 * @param archive_read ZIP archive reading handle
 * @param archive_write Disk writing handle
 * @param item Archive entry to extract
 * @return WD_RETZ on success, negative error code on failure
 */
static int extract_zip_entry(struct archive *archive_read,
						     struct archive *archive_write,
						     struct archive_entry *item)
{
		int ret;
		const void *buffer;  /* Data buffer pointer */
		size_t size;         /* Data buffer size */
		la_int64_t offset;   /* File offset */

		/* Write the entry header to disk (creates file/directory) */
		ret = archive_write_header(archive_write, item);
		if (ret != ARCHIVE_OK) {
				pr_error(stdout, "Write header error: %s", archive_error_string(archive_write));
				return -WD_RETN;
		}

		/* Copy file content if this entry has data */
		while (1) {
				/* Read a block of data from the ZIP entry */
				ret = archive_read_data_block(archive_read, &buffer, &size, &offset);
				/* Check for end of file */
				if (ret == ARCHIVE_EOF)
						break;
				/* Handle read errors */
				if (ret < ARCHIVE_OK) {
						pr_error(stdout, "Read data error: %s", archive_error_string(archive_read));
						return -WD_RETW;
				}

				/* Write the data block to disk */
				ret = archive_write_data_block(archive_write, buffer, size, offset);
				/* Handle write errors */
				if (ret < ARCHIVE_OK) {
						pr_error(stdout, "Write data error: %s", archive_error_string(archive_write));
						return -WD_RETH;
				}
		}

		/* Return success code */
		return WD_RETZ;
}

/**
 * Extracts a ZIP archive file to the specified destination directory
 * Handles path construction and error recovery
 *
 * @param zip_file Path to the ZIP archive file to extract
 * @param entry_dest Destination directory for extraction
 * @return WD_RETZ on success, negative error code on failure
 */
int wd_extract_zip(const char *zip_file, const char *entry_dest)
{
		struct archive *archive_read;     /* ZIP reading handle */
		struct archive *archive_write;    /* Disk writing handle */
		struct archive_entry *item;       /* Current archive entry */
		char full_path[1024 * 1024];     /* Buffer for full extraction path */
		int ret;                          /* Return status */
		int error_occurred = 0;           /* Error flag */

		/* Initialize archive handles */
		archive_read = archive_read_new();
		archive_write = archive_write_disk_new();

		/* Validate handle creation */
		if (!archive_read || !archive_write) {
				pr_error(stdout, "Failed to create archive handles");
				goto error;
		}

		/* Configure reader for ZIP format with all supported filters */
		archive_read_support_format_zip(archive_read);
		archive_read_support_filter_all(archive_read);

		/* Configure disk writer with basic options */
		archive_write_disk_set_options(archive_write, ARCHIVE_EXTRACT_TIME);
		archive_write_disk_set_standard_lookup(archive_write);

		/* Open the ZIP file with 1MB buffer */
		ret = archive_read_open_filename(archive_read, zip_file, 1024 * 1024);
		if (ret != ARCHIVE_OK) {
				pr_error(stdout, "Cannot open file: %s", archive_error_string(archive_read));
				goto error;
		}

		/* Process each entry in the ZIP archive */
		while (archive_read_next_header(archive_read, &item) == ARCHIVE_OK) {
				const char *entry_path = archive_entry_pathname(item);
				
				/* Debugging Notice */
				static int extract_notice = 0;
				static int always_extract_notice = 0;
				if (extract_notice == 0) {
					extract_notice = 1;
					pr_color(stdout, FCOLOUR_GREEN, "* create debugging extracting archive?");
					char *debug_extract = readline(" [y/n]: ");
					if (debug_extract) {
						if (debug_extract[0] == 'Y' || debug_extract[0] == 'y') {
							always_extract_notice = 1;
			    			printf(" * Extracting: %s\n", entry_path);
						}
					}
				}
				if (always_extract_notice) {
					printf(" * Extracting: %s\n", entry_path);
				}

				/* Build the full extraction path for this entry */
				build_extraction_path(entry_dest, entry_path, full_path, sizeof(full_path));
				/* Update the entry with the new full path */
				archive_entry_set_pathname(item, full_path);

				/* Extract the current entry */
				if (extract_zip_entry(archive_read, archive_write, item) != 0) {
						error_occurred = 1;
						break;
				}
		}

		/* Close archive handles */
		archive_read_close(archive_read);
		archive_write_close(archive_write);

		/* Free archive resources */
		archive_read_free(archive_read);
		archive_write_free(archive_write);

		/* Return success if no errors occurred */
		return error_occurred ? -WD_RETN : 0;

/* Error handling section */
error:
		/* Clean up archive handles */
		if (archive_read)
				archive_read_free(archive_read);
		if (archive_write)
				archive_write_free(archive_write);
		return -WD_RETN;
}

/**
 * Extracts archive files based on their type
 * Supports TAR, TAR.GZ, and ZIP formats
 *
 * @param filename Path to the archive file to extract
 * @return WD_RETZ on success, negative error code on failure
 */
void wd_extract_archive(const char *filename)
{
        /* Debugging Extract Archive Function */
#if defined (_DBG_PRINT)
		pr_color(stdout, FCOLOUR_YELLOW, "-DEBUGGING ");
	    printf("[function: %s | "
               "pretty function: %s | "
               "line: %d | "
               "file: %s | "
               "date: %s | "
               "time: %s | "
               "timestamp: %s | "
               "C standard: %ld | "
               "C version: %s | "
               "compiler version: %d | "
               "architecture: %s]:\n",
                __func__, __PRETTY_FUNCTION__,
                __LINE__, __FILE__,
                __DATE__, __TIME__,
                __TIMESTAMP__,
                __STDC_VERSION__,
                __VERSION__,
                __GNUC__,
#ifdef __x86_64__
                "x86_64");
#elif defined(__i386__)
                "i386");
#elif defined(__arm__)
                "ARM");
#elif defined(__aarch64__)
                "ARM64");
#else
                "Unknown");
#endif
#endif
		char output_path[WD_PATH_MAX];  /* Buffer for extraction path */
		size_t name_len;                /* Length of filename */

		/* Check for TAR archive formats */
		if (strstr(filename, ".tar.gz")) {
				printf(" Try Extracting TAR archive: %s\n", filename);
				char size_filename[WD_PATH_MAX];
			    wd_snprintf(size_filename, sizeof(size_filename), "%s", filename);
			    if (strstr(size_filename, ".tar.gz")) {
			        char *f_EXT = strstr(size_filename, ".tar.gz");
			        if (f_EXT)
			            *f_EXT = '\0';  /* Remove .tar.gz extension */
			    }
				wd_extract_tar(filename, size_filename);  /* Extract using TAR handler */
		} else if (strstr(filename, ".tar")) {
				printf(" Try Extracting TAR archive: %s\n", filename);
				wd_extract_tar(filename, NULL);  /* Extract using TAR handler */
		} else if (strstr(filename, ".zip")) {
				/* Handle ZIP archive extraction */
				printf(" Try Extracting ZIP archive: %s\n", filename);

				/* Remove .zip extension for output directory */
				name_len = strlen(filename);
				if (name_len > 4 &&
					!strncmp(filename + name_len - 4, ".zip", 4))
				{
						/* Copy filename without .zip extension */
						wd_strncpy(output_path, filename, name_len - 4);
						output_path[name_len - 4] = '\0';
				} else {
						/* Use full filename if no .zip extension found */
						wd_strcpy(output_path, filename);
				}
				wd_extract_zip(filename, output_path);  /* Extract using ZIP handler */
		} else {
				/* Unknown archive format - log and return error */
				pr_info(stdout, "Unknown archive type, skipping extraction: %s\n", filename);
				goto done;
		}

		sleep(1);
done:
		return;
}
