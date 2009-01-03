/* the Music Player Daemon (MPD)
 * Copyright (C) 2003-2007 by Warren Dukes (warren.dukes@gmail.com)
 * This project's homepage is: http://www.musicpd.org
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "utils.h"
#include "conf.h"

#include "../config.h"

#include <glib.h>

#include <assert.h>
#include <string.h>
#include <sys/types.h>
#include <fcntl.h>

#ifndef WIN32
#include <pwd.h>
#endif

#ifdef HAVE_IPV6
#include <sys/socket.h>
#endif

#ifdef WIN32
#include <windows.h>
#endif

void my_usleep(long usec)
{
#ifdef WIN32
	Sleep(usec / 1000);
#else
	struct timeval tv;

	tv.tv_sec = 0;
	tv.tv_usec = usec;

	select(0, NULL, NULL, NULL, &tv);
#endif
}

G_GNUC_MALLOC char *xstrdup(const char *s)
{
	char *ret = strdup(s);
	if (G_UNLIKELY(!ret))
		g_error("OOM: strdup");
	return ret;
}

/* borrowed from git :) */

G_GNUC_MALLOC void *xmalloc(size_t size)
{
	void *ret;

	assert(G_LIKELY(size));

	ret = malloc(size);
	if (G_UNLIKELY(!ret))
		g_error("OOM: malloc");
	return ret;
}

G_GNUC_MALLOC void *xrealloc(void *ptr, size_t size)
{
	void *ret = realloc(ptr, size);

	/* some C libraries return NULL when size == 0,
	 * make sure we get a free()-able pointer (free(NULL)
	 * doesn't work with all C libraries, either) */
	if (G_UNLIKELY(!ret && !size))
		ret = realloc(ptr, 1);

	if (G_UNLIKELY(!ret))
		g_error("OOM: realloc");
	return ret;
}

G_GNUC_MALLOC void *xcalloc(size_t nmemb, size_t size)
{
	void *ret;

	assert(G_LIKELY(nmemb && size));

	ret = calloc(nmemb, size);
	if (G_UNLIKELY(!ret))
		g_error("OOM: calloc");
	return ret;
}

char *parsePath(char *path)
{
#ifndef WIN32
	if (path[0] != '/' && path[0] != '~') {
		g_warning("\"%s\" is not an absolute path", path);
		return NULL;
	} else if (path[0] == '~') {
		size_t pos = 1;
		const char *home;
		char *newPath;

		if (path[1] == '/' || path[1] == '\0') {
			ConfigParam *param = getConfigParam(CONF_USER);
			if (param && param->value) {
				struct passwd *passwd = getpwnam(param->value);
				if (!passwd) {
					g_warning("no such user %s",
						  param->value);
					return NULL;
				}

				home = passwd->pw_dir;
			} else {
				home = g_get_home_dir();
				if (home == NULL) {
					g_warning("problems getting home "
						  "for current user");
					return NULL;
				}
			}
		} else {
			bool foundSlash = false;
			struct passwd *passwd;
			char *c;

			for (c = path + 1; *c != '\0' && *c != '/'; c++);
			if (*c == '/') {
				foundSlash = true;
				*c = '\0';
			}
			pos = c - path;

			passwd = getpwnam(path + 1);
			if (!passwd) {
				g_warning("user \"%s\" not found", path + 1);
				return NULL;
			}

			if (foundSlash)
				*c = '/';

			home = passwd->pw_dir;
		}

		newPath = xmalloc(strlen(home) + strlen(path + pos) + 1);
		strcpy(newPath, home);
		strcat(newPath, path + pos);
		return newPath;
	} else {
#endif
		return xstrdup(path);
#ifndef WIN32
	}
#endif
}

int set_nonblocking(int fd)
{
#ifdef WIN32
	u_long val = 0;

	return ioctlsocket(fd, FIONBIO, &val) == 0 ? 0 : -1;
#else
	int ret, flags;

	assert(fd >= 0);

	while ((flags = fcntl(fd, F_GETFL)) < 0 && errno == EINTR) ;
	if (flags < 0)
		return flags;

	flags |= O_NONBLOCK;
	while ((ret = fcntl(fd, F_SETFL, flags)) < 0 && errno == EINTR) ;
	return ret;
#endif
}

int stringFoundInStringArray(const char *const*array, const char *suffix)
{
	while (array && *array) {
		if (strcasecmp(*array, suffix) == 0)
			return 1;
		array++;
	}

	return 0;
}
