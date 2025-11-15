#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <archive.h>
#include <archive_entry.h>

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
 * Extracts a tar archive file to the current directory
 * Supports various tar formats and compression filters
 * 
 * @param tar_file Path to the tar archive file to extract
 * @return 0 on success, negative error code on failure
 */
int wd_extract_tar(const char *tar_file)
{
		struct archive *archive_read;   /* Archive reading handle */
		struct archive *archive_write;  /* Disk writing handle */
		struct archive_entry *item;     /* Current archive entry */
		int ret;                        /* Return status */

		/* Initialize archive reading and writing handles */
		archive_read = archive_read_new();
		archive_write = archive_write_disk_new();

		/* Check if archive handles were created successfully */
		if (!archive_read || !archive_write) {
				pr_error(stdout, "Failed to create archive handles");
				goto error;
		}

		/* Configure archive reader to support all formats and filters */
		archive_read_support_format_all(archive_read);
		archive_read_support_filter_all(archive_read);

		/* Set extraction options for disk writer */
		archive_write_disk_set_options(archive_write,
								       ARCHIVE_EXTRACT_TIME |        /* Preserve file times */
								       ARCHIVE_EXTRACT_PERM |        /* Preserve permissions */
								       ARCHIVE_EXTRACT_ACL |         /* Preserve ACLs */
								       ARCHIVE_EXTRACT_FFLAGS |      /* Preserve file flags */
								       ARCHIVE_EXTRACT_UNLINK |      /* Unlink existing files */
								       ARCHIVE_EXTRACT_SECURE_SYMLINKS |  /* Security: safe symlinks */
								       ARCHIVE_EXTRACT_SECURE_NODOTDOT |  /* Security: no ../ in paths */
								       ARCHIVE_EXTRACT_NO_OVERWRITE_NEWER); /* Don't overwrite newer files */

		/* Open the tar file for reading with 1MB buffer */
		ret = archive_read_open_filename(archive_read, tar_file, 1024 * 1024);
		if (ret != ARCHIVE_OK) {
				pr_error(stdout, "Cannot open file: %s", archive_error_string(archive_read));
				goto error;
		}

		/* Process each entry in the archive */
		while (1) {
				/* Read the next header from the archive */
				ret = archive_read_next_header(archive_read, &item);
				/* Check for end of archive */
				if (ret == ARCHIVE_EOF)
						break;
				/* Handle header reading errors */
				if (ret != ARCHIVE_OK) {
						pr_error(stdout, "Header error: %s", archive_error_string(archive_read));
						break;
				}

				/* Write the header to disk (creates directory structure) */
				ret = archive_write_header(archive_write, item);
				if (ret != ARCHIVE_OK) {
						pr_error(stdout, "Write header error: %s", archive_error_string(archive_write));
						break;
				}

				/* If this entry has file content, copy the data */
				if (archive_entry_size(item) > 0) {
						ret = arch_copy_data(archive_read, archive_write);
						if (ret != ARCHIVE_OK)
								break;
				}

				/* Complete the current entry write operation */
				archive_write_finish_entry(archive_write);
		}

		/* Close archive handles */
		archive_read_close(archive_read);
		archive_write_close(archive_write);

		/* Free archive resources */
		archive_read_free(archive_read);
		archive_write_free(archive_write);

		/* Return success if we reached end of file, error otherwise */
		return (ret == ARCHIVE_EOF) ? 0 : -__RETN;

/* Error handling section */
error:
		/* Clean up archive handles if they were created */
		if (archive_read)
				archive_read_free(archive_read);
		if (archive_write)
				archive_write_free(archive_write);
		return -__RETN;
}

/**
 * Builds the full extraction path for an archive entry
 * Combines destination directory with entry path, handling edge cases
 * 
 * @param dest_path Destination directory path (can be NULL or ".")
 * @param entry_path Path of the entry within the archive
 * @param out_path Output buffer for the full path
 * @param out_size Size of the output buffer
 */
static void build_extraction_path(const char *dest_path, const char *entry_path,
								  char *out_path, size_t out_size)
{
		/* Handle case where no destination path is specified */
		if (!dest_path || !strcmp(dest_path, ".")|| *dest_path == '\0') {
				/* Extract directly to current directory using entry path */
				wd_snprintf(out_path, out_size, "%s", entry_path);
		} else {
				/* Check if entry path already contains destination path */
				if (!strncmp(entry_path, dest_path, strlen(dest_path))) {
						/* Use entry path as-is if it already starts with dest path */
						wd_snprintf(out_path, out_size, "%s", entry_path);
				} else {
						/* Combine destination path with entry path */
						wd_snprintf(out_path, out_size, "%s/%s", dest_path, entry_path);
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
 * @return __RETZ on success, negative error code on failure
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
				return -__RETN;
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
						return -__RETW;
				}

				/* Write the data block to disk */
				ret = archive_write_data_block(archive_write, buffer, size, offset);
				/* Handle write errors */
				if (ret < ARCHIVE_OK) {
						pr_error(stdout, "Write data error: %s", archive_error_string(archive_write));
						return -__RETH;
				}
		}

		/* Return success code */
		return __RETZ;
}

/**
 * Extracts a ZIP archive file to the specified destination directory
 * Handles path construction and error recovery
 * 
 * @param zip_file Path to the ZIP archive file to extract
 * @param dest_path Destination directory for extraction
 * @return 0 on success, negative error code on failure
 */
int wd_extract_zip(const char *zip_file, const char *dest_path)
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

				/* Build the full extraction path for this entry */
				build_extraction_path(dest_path, entry_path, full_path, sizeof(full_path));
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
		return error_occurred ? -__RETN : 0;

/* Error handling section */
error:
		/* Clean up archive handles */
		if (archive_read)
				archive_read_free(archive_read);
		if (archive_write)
				archive_write_free(archive_write);
		return -__RETN;
}