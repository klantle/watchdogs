#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include <archive.h>
#include <archive_entry.h>

#include "wg_extra.h"
#include "wg_util.h"
#include "wg_archive.h"
#include "wg_curl.h"
#include "wg_unit.h"

static int arch_copy_data(struct archive *ar, struct archive *aw)
{
	size_t size;
	la_int64_t offset;
	int ret = -WG_RETW;
	const void *buffer;

	while (true) {
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

int wg_extract_tar(const char *tar_path, const char *entry_dest) {
	struct archive *a;
	struct archive *ext;
	struct archive_entry *entry;
	int flags;
	int r;

	flags = ARCHIVE_EXTRACT_TIME;
	flags |= ARCHIVE_EXTRACT_PERM;
	flags |= ARCHIVE_EXTRACT_ACL;
	flags |= ARCHIVE_EXTRACT_FFLAGS;

	a = archive_read_new();
	archive_read_support_format_all(a);
	archive_read_support_filter_all(a);

	ext = archive_write_disk_new();
	archive_write_disk_set_options(ext, flags);
	archive_write_disk_set_standard_lookup(ext);

	r = archive_read_open_filename(a, tar_path, 10240);
	if (r != ARCHIVE_OK) {
		fprintf(stderr, "Error opening archive: %s\n", archive_error_string(a));
		archive_read_free(a);
		archive_write_free(ext);
		return -WG_RETN;
	}

	while (true) {
		r = archive_read_next_header(a, &entry);
		if (r == ARCHIVE_EOF) {
			break;
		}
		if (r != ARCHIVE_OK) {
			fprintf(stderr, "Error reading header: %s\n", archive_error_string(a));
			archive_read_free(a);
			archive_write_free(ext);
			return -WG_RETN;
		}

		const char *entry_path = archive_entry_pathname(entry);

		static int extract_notice = 0;
		static int notice_extracting = 0;
		if (extract_notice == WG_RETZ) {
			extract_notice = 1;
			printf("\x1b[32m==> create debug extract archive?\x1b[0m\n");
			char *debug_extract = readline("   answer [y/n]: ");
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

		if (entry_dest != NULL && strlen(entry_dest) > 0) {
			char entry_new_path[1024];
			wg_mkdir(entry_dest);
			snprintf(entry_new_path, sizeof(entry_new_path), "%s/%s", entry_dest, entry_path);
			archive_entry_set_pathname(entry, entry_new_path);
		}

		r = archive_write_header(ext, entry);
		if (r != ARCHIVE_OK) {
			fprintf(stderr, "Error writing header for %s: %s\n",
					entry_path, archive_error_string(ext));
		} else {
			r = arch_copy_data(a, ext);
			if (r != ARCHIVE_OK && r != ARCHIVE_EOF) {
				fprintf(stderr, "Error copying data for %s\n", entry_path);
			}
		}

		r = archive_write_finish_entry(ext);
		if (r != ARCHIVE_OK) {
			fprintf(stderr, "Error finishing entry %s: %s\n",
					entry_path, archive_error_string(ext));
		}
	}

	archive_read_close(a);
	archive_read_free(a);
	archive_write_close(ext);
	archive_write_free(ext);

	return WG_RETZ;
}

static void build_extraction_path(const char *entry_dest, const char *entry_path,
								  char *out_path, size_t out_size)
{
	if (!entry_dest ||
		!strcmp(entry_dest, ".")||
		*entry_dest == '\0') {
		wg_snprintf(out_path, out_size, "%s", entry_path);
	} else {
		if (!strncmp(entry_path, entry_dest, strlen(entry_dest))) {
			wg_snprintf(out_path, out_size, "%s", entry_path);
		} else {
			wg_snprintf(out_path, out_size, "%s/%s", entry_dest, entry_path);
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
		return -WG_RETN;
	}

	while (true) {
		ret = archive_read_data_block(archive_read, &buffer, &size, &offset);
		if (ret == ARCHIVE_EOF)
			break;
		if (ret < ARCHIVE_OK) {
			pr_error(stdout, "Read data error: %s", archive_error_string(archive_read));
			return -WG_RETW;
		}

		ret = archive_write_data_block(archive_write, buffer, size, offset);
		if (ret < ARCHIVE_OK) {
			pr_error(stdout, "Write data error: %s", archive_error_string(archive_write));
			return -WG_RETH;
		}
	}

	return WG_RETZ;
}

int wg_extract_zip(const char *zip_file, const char *entry_dest)
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

		static int extract_notice = 0;
		static int notice_extracting = 0;
		if (extract_notice == WG_RETZ) {
			extract_notice = 1;
			printf("\x1b[32m==> create debug extract archive?\x1b[0m\n");
			char *debug_extract = readline("   answer [y/n]: ");
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

		build_extraction_path(entry_dest, entry_path, full_path, sizeof(full_path));
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

	return error_occurred ? -WG_RETN : 0;

error:
	if (archive_read)
		archive_read_free(archive_read);
	if (archive_write)
		archive_write_free(archive_write);
	return -WG_RETN;
}

void wg_extract_archive(const char *filename)
{
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
	char output_path[WG_PATH_MAX];
	size_t name_len;

	if (strstr(filename, ".tar.gz")) {
		printf(" Try Extracting TAR archive: %s\n", filename);
		char size_filename[WG_PATH_MAX];
		wg_snprintf(size_filename, sizeof(size_filename), "%s", filename);
		if (strstr(size_filename, ".tar.gz")) {
			char *ext = strstr(size_filename, ".tar.gz");
			if (ext)
				*ext = '\0';
		}
		wg_extract_tar(filename, size_filename);
	} else if (strstr(filename, ".tar")) {
		printf(" Try Extracting TAR archive: %s\n", filename);
		wg_extract_tar(filename, NULL);
	} else if (strstr(filename, ".zip")) {
		printf(" Try Extracting ZIP archive: %s\n", filename);

		name_len = strlen(filename);
		if (name_len > 4 &&
			!strncmp(filename + name_len - 4, ".zip", 4))
		{
			wg_strncpy(output_path, filename, name_len - 4);
			output_path[name_len - 4] = '\0';
		} else {
			wg_strcpy(output_path, filename);
		}
		wg_extract_zip(filename, output_path);
	} else {
		pr_info(stdout, "Unknown archive type, skipping extraction: %s\n", filename);
		goto done;
	}

	sleep(1);
done:
	return;
}