#ifndef PACKAGE_H
#define PACKAGE_H

struct package_version_info {
		char key;
		const char *name;
		const char *linux_url;
		const char *linux_file;
		const char *windows_url;
		const char *windows_file;
};

int wg_install_pawncc(const char *platform);
int wg_install_server(const char *platform);

#endif