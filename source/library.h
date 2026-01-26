/*-
 * Copyright (c) 2026 Watchdogs Team and contributors
 * All rights reserved. under The 2-Clause BSD License
 * See COPYING or https://opensource.org/license/bsd-2-clause
 */

#ifndef PACKAGE_H
#define PACKAGE_H

struct library_version_info {
		char key;
		const char *name;
		const char *linux_url;
		const char *linux_file;
		const char *windows_url;
		const char *windows_file;
};

extern bool installing_pawncc;

int dog_install_pawncc(const char *platform);
int dog_install_server(const char *platform);

#endif
