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

static int arch_copy_data(struct archive *ar, struct archive *aw)
{
		int ret;
		const void *buffer;
		size_t size;
		la_int64_t offset;

		while (1) {
				ret = archive_read_data_block(ar, &buffer, &size, &offset);
				if (ret == ARCHIVE_EOF)
						return ARCHIVE_OK;
				if (ret != ARCHIVE_OK) {
						pr_error(stdout, "Read error: %s", archive_error_string(ar));
						return ret;
				}

				ret = archive_write_data_block(aw, buffer, size, offset);
				if (ret != ARCHIVE_OK) {
						pr_error(stdout, "Write error: %s", archive_error_string(aw));
						return ret;
				}
		}
}

int wd_extract_tar(const char *tar_file)
{
		struct archive *archive_read;
		struct archive *archive_write;
		struct archive_entry *item;
		int ret;

		archive_read = archive_read_new();
		archive_write = archive_write_disk_new();

		if (!archive_read || !archive_write) {
				pr_error(stdout, "Failed to create archive handles");
				goto error;
		}

		archive_read_support_format_all(archive_read);
		archive_read_support_filter_all(archive_read);

		archive_write_disk_set_options(archive_write,
								       ARCHIVE_EXTRACT_TIME |
								       ARCHIVE_EXTRACT_PERM |
								       ARCHIVE_EXTRACT_ACL |
								       ARCHIVE_EXTRACT_FFLAGS |
								       ARCHIVE_EXTRACT_UNLINK |
								       ARCHIVE_EXTRACT_SECURE_SYMLINKS |
								       ARCHIVE_EXTRACT_SECURE_NODOTDOT |
								       ARCHIVE_EXTRACT_NO_OVERWRITE_NEWER);

		ret = archive_read_open_filename(archive_read, tar_file, 1024 * 1024);
		if (ret != ARCHIVE_OK) {
				pr_error(stdout, "Cannot open file: %s", archive_error_string(archive_read));
				goto error;
		}

		while (1) {
				ret = archive_read_next_header(archive_read, &item);
				if (ret == ARCHIVE_EOF)
						break;
				if (ret != ARCHIVE_OK) {
						pr_error(stdout, "Header error: %s", archive_error_string(archive_read));
						break;
				}

				ret = archive_write_header(archive_write, item);
				if (ret != ARCHIVE_OK) {
						pr_error(stdout, "Write header error: %s", archive_error_string(archive_write));
						break;
				}

				if (archive_entry_size(item) > 0) {
						ret = arch_copy_data(archive_read, archive_write);
						if (ret != ARCHIVE_OK)
								break;
				}

				archive_write_finish_entry(archive_write);
		}

		archive_read_close(archive_read);
		archive_write_close(archive_write);

		archive_read_free(archive_read);
		archive_write_free(archive_write);

		return (ret == ARCHIVE_EOF) ? 0 : -__RETN;

error:
		if (archive_read)
				archive_read_free(archive_read);
		if (archive_write)
				archive_write_free(archive_write);
		return -__RETN;
}

static void build_extraction_path(const char *dest_path, const char *entry_path,
								  char *out_path, size_t out_size)
{
		if (!dest_path || !strcmp(dest_path, ".")|| *dest_path == '\0') {
				snprintf(out_path, out_size, "%s", entry_path);
		} else {
				if (!strncmp(entry_path, dest_path, strlen(dest_path))) {
						snprintf(out_path, out_size, "%s", entry_path);
				} else {
						snprintf(out_path, out_size, "%s/%s", dest_path, entry_path);
				}
		}
}

static int extract_zip_entry(struct archive *archive_read,
						     struct archive *archive_write,
						     struct archive_entry *item)
{
		int ret;
		const void *buffer;
		size_t size;
		la_int64_t offset;

		ret = archive_write_header(archive_write, item);
		if (ret != ARCHIVE_OK) {
				pr_error(stdout, "Write header error: %s", archive_error_string(archive_write));
				return -__RETN;
		}

		while (1) {
				ret = archive_read_data_block(archive_read, &buffer, &size, &offset);
				if (ret == ARCHIVE_EOF)
						break;
				if (ret < ARCHIVE_OK) {
						pr_error(stdout, "Read data error: %s", archive_error_string(archive_read));
						return -__RETW;
				}

				ret = archive_write_data_block(archive_write, buffer, size, offset);
				if (ret < ARCHIVE_OK) {
						pr_error(stdout, "Write data error: %s", archive_error_string(archive_write));
						return -__RETH;
				}
		}

		return __RETZ;
}

int wd_extract_zip(const char *zip_file, const char *dest_path)
{
		struct archive *archive_read;
		struct archive *archive_write;
		struct archive_entry *item;
		char full_path[1024 * 1024];
		int ret;
		int error_occurred = 0;

		archive_read = archive_read_new();
		archive_write = archive_write_disk_new();

		if (!archive_read || !archive_write) {
				pr_error(stdout, "Failed to create archive handles");
				goto error;
		}

		archive_read_support_format_zip(archive_read);
		archive_read_support_filter_all(archive_read);

		archive_write_disk_set_options(archive_write, ARCHIVE_EXTRACT_TIME);
		archive_write_disk_set_standard_lookup(archive_write);

		ret = archive_read_open_filename(archive_read, zip_file, 1024 * 1024);
		if (ret != ARCHIVE_OK) {
				pr_error(stdout, "Cannot open file: %s", archive_error_string(archive_read));
				goto error;
		}

		while (archive_read_next_header(archive_read, &item) == ARCHIVE_OK) {
				const char *entry_path = archive_entry_pathname(item);

				build_extraction_path(dest_path, entry_path, full_path, sizeof(full_path));
				archive_entry_set_pathname(item, full_path);

				if (extract_zip_entry(archive_read, archive_write, item) != 0) {
						error_occurred = 1;
						break;
				}
		}

		archive_read_close(archive_read);
		archive_write_close(archive_write);

		archive_read_free(archive_read);
		archive_write_free(archive_write);

		return error_occurred ? -__RETN : 0;

error:
		if (archive_read)
				archive_read_free(archive_read);
		if (archive_write)
				archive_write_free(archive_write);
		return -__RETN;
}
