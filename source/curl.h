/*-
 * Copyright (c) 2026 Watchdogs Team and contributors
 * All rights reserved. under The 2-Clause BSD License
 * See COPYING or https://opensource.org/license/bsd-2-clause
 */

#ifndef CURL_H
#define CURL_H

#include "utils.h"

struct buf {
	    char *data;
	    size_t len;
		size_t allocated;
};

struct package_curl_buffer {
        char *data;
        size_t size;
};

struct memory_struct {
		char *memory;
		size_t size;
		size_t allocated;
};

#define DOG_CURL_RESPONSE_OK (200)
#define MAX_NUM_SITES (80)
#define MAX_USERNAME_LEN	100
#define MAX_VARIATIONS		100
#define USERNAME_PLACEHOLDER "%s"

typedef struct {
    const char *site_name;
    const char *url_template;
} SocialSite;

static const SocialSite social_site_list[MAX_NUM_SITES] = {
{"GitHub", "https://github.com/" USERNAME_PLACEHOLDER},
{"GitLab", "https://gitlab.com/" USERNAME_PLACEHOLDER},
{"Gitea", "https://gitea.io/" USERNAME_PLACEHOLDER},
{"SourceForge", "https://sourceforge.net/u/" USERNAME_PLACEHOLDER},
{"Bitbucket", "https://bitbucket.org/" USERNAME_PLACEHOLDER},
{"Stack Overflow", "https://stackoverflow.com/users/" USERNAME_PLACEHOLDER},
{"DEV Community", "https://dev.to/" USERNAME_PLACEHOLDER},
{"HackerRank", "https://www.hackerrank.com/" USERNAME_PLACEHOLDER},
{"LeetCode", "https://leetcode.com/" USERNAME_PLACEHOLDER},
{"Codewars", "https://www.codewars.com/users/" USERNAME_PLACEHOLDER},
{"CodePen", "https://codepen.io/" USERNAME_PLACEHOLDER},
{"JSFiddle", "https://jsfiddle.net/user/" USERNAME_PLACEHOLDER},
{"Replit", "https://replit.com/@" USERNAME_PLACEHOLDER},
{"Medium", "https://medium.com/@" USERNAME_PLACEHOLDER},
{"Substack", "https://" USERNAME_PLACEHOLDER ".substack.com"},
{"WordPress", "https://" USERNAME_PLACEHOLDER ".wordpress.com"},
{"Blogger", "https://" USERNAME_PLACEHOLDER ".blogspot.com"},
{"Tumblr", "https://" USERNAME_PLACEHOLDER ".tumblr.com"},
{"Mastodon", "https://mastodon.social/@" USERNAME_PLACEHOLDER},
{"Bluesky", "https://bsky.app/profile/" USERNAME_PLACEHOLDER},
{"Threads", "https://www.threads.net/@" USERNAME_PLACEHOLDER},
{"SlideShare", "https://www.slideshare.net/" USERNAME_PLACEHOLDER},
{"Speaker Deck", "https://speakerdeck.com/" USERNAME_PLACEHOLDER},
{"Reddit", "https://www.reddit.com/user/" USERNAME_PLACEHOLDER},
{"Discord", "https://discord.com/users/" USERNAME_PLACEHOLDER},
{"Keybase", "https://keybase.io/" USERNAME_PLACEHOLDER},
{"Gravatar", "https://gravatar.com/" USERNAME_PLACEHOLDER},
{"Letterboxd", "https://letterboxd.com/" USERNAME_PLACEHOLDER},
{"Trello", "https://trello.com/" USERNAME_PLACEHOLDER},
{"Linktree", "https://linktr.ee/" USERNAME_PLACEHOLDER},
{ NULL, NULL }
};

extern bool compiling_gamemode;

void curl_verify_cacert_pem(CURL *curl);

void buf_init(struct buf *b);
void buf_free(struct buf *b);

size_t write_callbacks(void *ptr, size_t size, size_t nmemb, void *userdata);
void memory_struct_init(struct memory_struct *mem);
void memory_struct_free(struct memory_struct *mem);
size_t write_memory_callback(void *contents, size_t size, size_t nmemb, void *userp);

void tracker_discrepancy(const char *base,
                         char variations[][MAX_USERNAME_LEN],
                         int *variation_count);
void tracking_username(CURL *curl, const char *username);

int dog_download_file(const char *url, const char *fname);

#endif
