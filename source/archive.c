/*-
 * Copyright (c) 2026 Watchdogs Team and contributors
 * All rights reserved. under The 2-Clause BSD License
 * See COPYING or https://opensource.org/license/bsd-2-clause
 */

#include  "utils.h"
#include  "archive.h"
#include  "curl.h"
#include  "debug.h"
#include  "units.h"

static void
arch_extraction_path(const char *dest, const char *path,
    char *out, size_t out_size)
{
	if (strlen(dest) < 1 ||
	    strcmp(dest, ".") == 0 ||
	    strcmp(dest, "none") == 0 ||
	    strcmp(dest, "root") == 0)
	{
		snprintf(out, out_size, "%s", path);
	} else {
		if (strncmp(path, dest, strlen(dest)) == 0) {
			snprintf(out, out_size, "%s", path);
		} else {
			snprintf(out, out_size, "%s" "%s" "%s",
			    dest, _PATH_STR_SEP_POSIX, path);
		}
	}
}

static int
arch_copy_data(struct archive *ar, struct archive *aw)
{
	size_t		 size;
	la_int64_t	 offset;
	int		 ret = -2;
	const void	*buffer;

	while (true) {
		ret = archive_read_data_block(ar, &buffer, &size, &offset);
		if (ret == ARCHIVE_EOF)
			return (ARCHIVE_OK);
		if (ret != ARCHIVE_OK) {
			pr_warning(stdout,
			    "arch_copy_data getting error "
			    "(read error): %s",
			    archive_error_string(ar));
			return (ret);
		}

		ret = archive_write_data_block(aw, buffer, size, offset);
		if (ret != ARCHIVE_OK) {
			pr_warning(stdout,
			    "arch_copy_data getting error "
			    "(write error): %s",
			    archive_error_string(aw));
			return (ret);
		}
	}
}

int
compress_to_archive(const char *archive_path,
    const char **file_paths,
    int raw_num_files,
    CompressionFormat format)
{
	struct archive	*archive;
	struct archive_entry *entry;
	char		 buffer[DOG_MORE_MAX_PATH];
	size_t		 len;
	int		 fd;
	int		 ret = 0;
	struct stat	 fd_stat;

	archive = archive_write_new();
	if (archive == NULL) {
		pr_error(stdout, "failed to creating archive object..");
		minimal_debugging();
		return (-1);
	}

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
		pr_error(stdout,
		    "unsupport compression format.. default.: zip, tar, gz, bz2, xz");
		archive_write_free(archive);
		return (-1);
	}

	ret = archive_write_open_filename(archive, archive_path);
	if (ret != ARCHIVE_OK) {
		pr_error(stdout, "failed to open the archive: %s",
		    archive_path);
		minimal_debugging();
		archive_write_free(archive);
		return (-1);
	}

	for (int i = 0; i < raw_num_files; i++) {
		const char	*filename = file_paths[i];

		fd = open(filename, O_RDONLY);
		if (fd < 0) {
			pr_error(stdout, "failed to open the file: %s..: %s",
			    filename, strerror(errno));
			minimal_debugging();
			ret = -1;
			continue;
		}

		if (fstat(fd, &fd_stat) != 0) {
			pr_error(stdout, "failed to fstat file: %s..: %s",
			    filename, strerror(errno));
			minimal_debugging();
			close(fd);
			ret = -1;
			continue;
		}

		if (!file_regular(archive_path)) {
			if (S_ISDIR(fd_stat.st_mode)) {
				close(fd);
				ret = -2;
				goto fallback;
			}
			pr_warning(stdout, "the %s is not a regular file!..",
			    filename);
			minimal_debugging();
			close(fd);
			ret = -1;
			continue;
		}

		entry = archive_entry_new();
		if (entry == NULL) {
			pr_error(stdout,
			    "failed to creating archive entry for: %s",
			    filename);
			minimal_debugging();
			close(fd);
			ret = -1;
			continue;
		}

		archive_entry_set_pathname(entry, filename);
		archive_entry_set_size(entry, fd_stat.st_size);
		archive_entry_set_filetype(entry, AE_IFREG);
		archive_entry_set_perm(entry, fd_stat.st_mode);
		archive_entry_set_mtime(entry, fd_stat.st_mtime, 0);
		archive_entry_set_atime(entry, fd_stat.st_atime, 0);
		archive_entry_set_ctime(entry, fd_stat.st_ctime, 0);

		ret = archive_write_header(archive, entry);
		if (ret != ARCHIVE_OK) {
			pr_error(stdout,
			    "failed to write header for: %s..: %s",
			    filename, archive_error_string(archive));
			minimal_debugging();
			archive_entry_free(entry);
			close(fd);
			ret = -1;
			continue;
		}

		while ((len = read(fd, buffer, sizeof(buffer))) > 0) {
			ssize_t	 written = archive_write_data(archive,
			    buffer, len);

			if (written < 0) {
				pr_error(stdout,
				    "failed to write data for: %s..: %s",
				    filename, archive_error_string(archive));
				minimal_debugging();
				ret = -1;
				break;
			}
			if (written != len) {
				pr_error(stdout, "partial write for: %s",
				    filename);
				minimal_debugging();
				ret = -1;
				break;
			}
		}

		if (len < 0) {
			pr_error(stdout,
			    "failed to trying read file: %s..: %s",
			    filename, strerror(errno));
			minimal_debugging();
			ret = -1;
		}

		close(fd);
		archive_entry_free(entry);

		if (ret != 0 && ret != -2) {
			break;
		}
	}

	archive_write_close(archive);
	archive_write_free(archive);

fallback:
	if (ret == -2) {
		ret = compress_directory(archive_path, file_paths[0], format);
	}

	return (ret);
}

int
dog_path_recursive(struct archive *archive, const char *root, const char *path)
{
	struct archive_entry *entry = NULL;
	struct stat		 path_stat;
	char			 full_path[DOG_MAX_PATH * 2];
	int			 fd = -1;
	struct stat		 fd_stat;
	size_t			 read_len;
	char			 buffer[DOG_MORE_MAX_PATH];
	DIR			*dirp = NULL;
	struct dirent		*dent;
	char			 child_path[DOG_MAX_PATH];

	snprintf(full_path, sizeof(full_path),
	    "%s" "%s" "%s", root, _PATH_STR_SEP_POSIX, path);

	#ifdef DOG_WINDOWS
	if (stat(full_path, &path_stat) != 0) {
		pr_error(stdout, "stat failed..: %s..: %s",
		    full_path, strerror(errno));
		minimal_debugging();
		return (-1);
	}
	#else
	if (lstat(full_path, &path_stat) != 0) {
		pr_error(stdout, "lstat failed..: %s..: %s",
		    full_path, strerror(errno));
		minimal_debugging();
		return (-1);
	}
	#endif

	if (S_ISREG(path_stat.st_mode)) {
	#ifdef DOG_WINDOWS
		fd = open(full_path, O_RDONLY|O_BINARY);
	#else
	#ifdef O_NOFOLLOW
	#ifdef O_CLOEXEC
		fd = open(full_path, O_RDONLY|O_NOFOLLOW|O_CLOEXEC);
	#else
		fd = open(full_path, O_RDONLY|O_NOFOLLOW);
	#endif
	#else
	#ifdef O_CLOEXEC
		fd = open(full_path, O_RDONLY|O_CLOEXEC);
	#else
		fd = open(full_path, O_RDONLY);
	#endif
	#endif
	#endif

		if (fd == -1) {
			pr_error(stdout, "open failed..: %s..: %s",
			    full_path, strerror(errno));
			minimal_debugging();
			return (-1);
		}

		if (fstat(fd, &fd_stat) != 0) {
			pr_error(stdout, "fstat failed..: %s..: %s",
			    full_path, strerror(errno));
			minimal_debugging();
			close(fd);
			return (-1);
		}

		if (!file_regular(full_path)) {
			pr_warning(stdout, "the %s is not a regular file!..",
			    full_path);
			minimal_debugging();
			close(fd);
			return (-1);
		}

		if (path_stat.st_ino != fd_stat.st_ino ||
		    path_stat.st_dev != fd_stat.st_dev) {
			pr_warning(stdout,
			    "the %s changes during processing..",
			    full_path);
			minimal_debugging();
			close(fd);
			return (-1);
		}

		path_stat = fd_stat;

		entry = archive_entry_new();
		if (!entry) {
			pr_error(stdout,
			    "failed to creating archive entry for: %s",
			    full_path);
			minimal_debugging();
			close(fd);
			return (-1);
		}

		archive_entry_set_pathname(entry, path);
		archive_entry_copy_stat(entry, &path_stat);

		if (archive_write_header(archive, entry) != ARCHIVE_OK) {
			pr_error(stdout, "archive_write_header failed..: %s",
			    archive_error_string(archive));
			minimal_debugging();
			archive_entry_free(entry);
			close(fd);
			return (-1);
		}

		while ((read_len = read(fd, buffer, sizeof(buffer))) > 0) {
			if (archive_write_data(archive, buffer, read_len) < 0) {
				pr_error(stdout,
				    "archive_write_data failed..: %s",
				    archive_error_string(archive));
				minimal_debugging();
				archive_entry_free(entry);
				close(fd);
				return (-1);
			}
		}

		if (read_len < 0) {
			pr_error(stdout, "failed to read..: %s..: %s",
			    full_path, strerror(errno));
			minimal_debugging();
		}

		close(fd);
		archive_entry_free(entry);
		return ((read_len < 0) ? -1 : 0);
	}

	entry = archive_entry_new();
	if (!entry) {
		pr_error(stdout,
		    "failed to creating archive entry for: %s", full_path);
		minimal_debugging();
		return (-1);
	}

	archive_entry_set_pathname(entry, path);
	archive_entry_copy_stat(entry, &path_stat);

	if (archive_write_header(archive, entry) != ARCHIVE_OK) {
		pr_error(stdout, "archive_write_header failed..: %s",
		    archive_error_string(archive));
		minimal_debugging();
		archive_entry_free(entry);
		return (-1);
	}

	archive_entry_free(entry);

	if (S_ISDIR(path_stat.st_mode)) {
		dirp = opendir(full_path);
		if (!dirp) {
			pr_error(stdout, "failed to opendir: %s..: %s",
			    full_path, strerror(errno));
			minimal_debugging();
			return (-1);
		}

		while ((dent = readdir(dirp)) != NULL) {
			if (dog_dot_or_dotdot(dent->d_name))
				continue;

			snprintf(child_path, sizeof(child_path),
			    "%s" "%s" "%s",
			    path, _PATH_STR_SEP_POSIX, dent->d_name);

			if (dog_path_recursive(archive, root, child_path) != 0) {
				closedir(dirp);
				return (-1);
			}
		}

		closedir(dirp);
	}

	return (0);
}

int
compress_directory(const char *archive_path,
    const char *dir_path,
    CompressionFormat format)
{
	struct archive	*a;
	int		 ret;

	a = archive_write_new();

	if (!a)
		return (-1);

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
		pr_error(stdout,
		    "unsupport compression format.. default.: zip, tar, gz, bz2, xz");
		archive_write_free(a);
		return (-1);
	}

	ret = archive_write_open_filename(a, archive_path);
	if (ret != ARCHIVE_OK) {
		pr_error(stdout, "failed to open the archive: %s",
		    archive_path);
		minimal_debugging();
		archive_write_free(a);
		return (-1);
	}

	dog_path_recursive(a, dir_path, "");

	archive_write_close(a);
	archive_write_free(a);

	return (0);
}

int
dog_extract_tar(const char *tar_path, const char *entry_dest)
{
	int		 r;
	int		 flags;
	struct archive	*a;
	struct archive	*ext;
	struct archive_entry *entry;

	flags = ARCHIVE_EXTRACT_TIME |
	    ARCHIVE_EXTRACT_PERM |
	    ARCHIVE_EXTRACT_ACL |
	    ARCHIVE_EXTRACT_FFLAGS;

	a = archive_read_new();
	archive_read_support_format_all(a);
	archive_read_support_filter_all(a);

	ext = archive_write_disk_new();
	archive_write_disk_set_options(ext, flags);
	archive_write_disk_set_standard_lookup(ext);

	r = archive_read_open_filename(a, tar_path, 10240);
	if (r != ARCHIVE_OK) {
		pr_error(stdout, "failed to opening the archive: %s..: %s",
		    tar_path, archive_error_string(a));
		minimal_debugging();
		archive_read_free(a);
		archive_write_free(ext);
		return (-1);
	}

	while (true) {
		r = archive_read_next_header(a, &entry);
		if (r == ARCHIVE_EOF) {
			break;
		}
		if (r != ARCHIVE_OK) {
			pr_error(stdout, "failed to reading header: %s..: %s",
			    tar_path, archive_error_string(a));
			minimal_debugging();
			archive_read_free(a);
			archive_write_free(ext);
			return (-1);
		}

		const char	*entry_path = NULL;
		entry_path = archive_entry_pathname(entry);

#if defined(_DBG_PRINT)
		printf(" * Extracting: %s\n", entry_path);
		fflush(stdout);
#endif
		if (entry_dest != NULL && strlen(entry_dest) > 0) {
			char	 entry_new_path[1024];
			dog_mkdir_recursive(entry_dest);
			snprintf(entry_new_path, sizeof(entry_new_path),
			    "%s" "%s" "%s", entry_dest,
			    _PATH_STR_SEP_POSIX, entry_path);
			archive_entry_set_pathname(entry, entry_new_path);
		}

		r = archive_write_header(ext, entry);
		if (r != ARCHIVE_OK) {
			pr_error(stdout,
			    "failed to writing header for: %s..: %s",
			    entry_path, archive_error_string(ext));
			minimal_debugging();
		} else {
			r = arch_copy_data(a, ext);
			if (r != ARCHIVE_OK && r != ARCHIVE_EOF) {
				pr_error(stdout, "error to copy data for: %s",
				    entry_path);
				minimal_debugging();
			}
		}

		r = archive_write_finish_entry(ext);
		if (r != ARCHIVE_OK) {
			pr_error(stdout,
			    "failed to finishing entry: %s..: %s",
			    entry_path, archive_error_string(ext));
			minimal_debugging();
		}
	}

	archive_read_close(a);
	archive_read_free(a);
	archive_write_close(ext);
	archive_write_free(ext);

	return (0);
}

static int
extract_zip_entry(struct archive *archive_read,
    struct archive *archive_write,
    struct archive_entry *item)
{
	int		 ret;
	const void	*buffer;
	size_t		 size;
	la_int64_t	 offset;

	ret = archive_write_header(archive_write, item);
	if (ret != ARCHIVE_OK) {
		pr_error(stdout, "failed to write header: %s",
		    archive_error_string(archive_write));
		minimal_debugging();
		return (-1);
	}

	while (true) {
		ret = archive_read_data_block(archive_read, &buffer,
		    &size, &offset);
		if (ret == ARCHIVE_EOF)
			break;
		if (ret < ARCHIVE_OK) {
			pr_error(stdout, "failed to read data: %s",
			    archive_error_string(archive_write));
			minimal_debugging();
			return (-2);
		}

		ret = archive_write_data_block(archive_write, buffer,
		    size, offset);
		if (ret < ARCHIVE_OK) {
			pr_error(stdout, "failed to write data: %s",
			    archive_error_string(archive_write));
			minimal_debugging();
			pr_error(stdout, "Write data error: %s",
			    archive_error_string(archive_write));
			return (-3);
		}
	}

	return (0);
}

int
dog_extract_zip(const char *zip_file, const char *entry_dest)
{
	struct archive	*archive_read;
	struct archive	*archive_write;
	struct archive_entry *item;
	char		 paths[DOG_MAX_PATH];
	int		 ret;
	int		 error_occurred = 0;

	archive_read = archive_read_new();
	archive_write = archive_write_disk_new();

	if (!archive_read || !archive_write) {
		pr_error(stdout, "failed to creating archive handler");
		minimal_debugging();
		goto error;
	}

	archive_read_support_format_zip(archive_read);
	archive_read_support_filter_all(archive_read);

	archive_write_disk_set_options(archive_write, ARCHIVE_EXTRACT_TIME);
	archive_write_disk_set_standard_lookup(archive_write);

	ret = archive_read_open_filename(archive_read, zip_file, 1024 * 1024);
	if (ret != ARCHIVE_OK) {
		pr_error(stdout, "cannot open the archive: %s",
		    archive_error_string(archive_read));
		minimal_debugging();
		goto error;
	}

	while (archive_read_next_header(archive_read, &item) == ARCHIVE_OK) {
		const char	*entry_path;
		entry_path = archive_entry_pathname(item);

#if defined(_DBG_PRINT)
		printf(" * Extracting: %s\n", entry_path);
		fflush(stdout);
#endif
		arch_extraction_path(entry_dest, entry_path, paths,
		    sizeof(paths));
		archive_entry_set_pathname(item, paths);

		if (extract_zip_entry(archive_read, archive_write, item) != 0) {
			error_occurred = 1;
			break;
		}
	}

	archive_read_close(archive_read);
	archive_write_close(archive_write);

	archive_read_free(archive_read);
	archive_write_free(archive_write);

	return (error_occurred ? -1 : 0);

error:
	if (archive_read)
		archive_read_free(archive_read);
	if (archive_write)
		archive_write_free(archive_write);
	return (-1);
}

void
destroy_arch_dir(const char *filename)
{
    if (!filename)
        return;

    pr_info(stdout, "Removing: %s..", filename);

	#ifdef DOG_WINDOWS
    DWORD attr = GetFileAttributesA(filename);
    if (attr == INVALID_FILE_ATTRIBUTES)
        return;

    if (attr & FILE_ATTRIBUTE_DIRECTORY) {
        SHFILEOPSTRUCTA op;
        char path[DOG_PATH_MAX];

        ZeroMemory(&op, sizeof(op));
        snprintf(path, sizeof(path), "%s%c%c", filename, '\0', '\0');

        op.wFunc = FO_DELETE;
        op.pFrom = path;
        op.fFlags = FOF_NO_UI | FOF_SILENT | FOF_NOCONFIRMATION;
        SHFileOperationA(&op);
    } else {
        DeleteFileA(filename);
    }
	#else
    struct stat st;
    if (lstat(filename, &st) != 0)
        return;

    if (S_ISDIR(st.st_mode)) {
        pid_t pid = fork();
        if (pid == 0) {
            execlp("rm", "rm", "-rf", filename, NULL);
            _exit(127);
        }
        waitpid(pid, NULL, 0);
    } else {
        int fd = open(filename, O_RDWR);
		if (fd != -1) unlink(filename);
    }
	#endif
}

int
is_archive_file(const char *filename)
{
	if (strend(filename, ".zip", true) ||
	    strend(filename, ".tar", true) ||
	    strend(filename, ".tar.gz", true)) {
		return (1);
	}
	return (0);
}

void
dog_extract_archive(const char *filename, const char *dir)
{
	if (dir_exists(".watchdogs") == 0)
		MKDIR(".watchdogs");

	if (!is_archive_file(filename)) {
		pr_warning(stdout,
			"File %s is not an archive",
		    filename);
		return;
	}

	pr_color(stdout, DOG_COL_CYAN,
	    " Try Extracting %s archive file...\n", filename);
	fflush(stdout);

	if (strend(filename, ".tar.gz", true)) {
		dog_extract_tar(filename, dir);
	} else if (strend(filename, ".tar", true)) {
		dog_extract_tar(filename, dir);
	} else if (strend(filename, ".zip", true)) {
		dog_extract_zip(filename, dir);
	} else {
		pr_info(stdout, "unknown archive: %s\n", filename);
	}

	return;
}
