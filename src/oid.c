/*
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2,
 * as published by the Free Software Foundation.
 *
 * In addition to the permissions in the GNU General Public License,
 * the authors give you unlimited permission to link the compiled
 * version of this file into combinations with other programs,
 * and to distribute those combinations without any restriction
 * coming from the use of this file.  (The General Public License
 * restrictions do apply in other respects; for example, they cover
 * modification of the file, and distribution when not linked into
 * a combined executable.)
 *
 * This file is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; see the file COPYING.  If not, write to
 * the Free Software Foundation, 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include "common.h"
#include "git2/oid.h"
#include "repository.h"
#include <string.h>

static signed char from_hex[] = {
-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, /* 00 */
-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, /* 10 */
-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, /* 20 */
 0,  1,  2,  3,  4,  5,  6,  7,  8,  9, -1, -1, -1, -1, -1, -1, /* 30 */
-1, 10, 11, 12, 13, 14, 15, -1, -1, -1, -1, -1, -1, -1, -1, -1, /* 40 */
-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, /* 50 */
-1, 10, 11, 12, 13, 14, 15, -1, -1, -1, -1, -1, -1, -1, -1, -1, /* 60 */
-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, /* 70 */
-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, /* 80 */
-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, /* 90 */
-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, /* a0 */
-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, /* b0 */
-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, /* c0 */
-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, /* d0 */
-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, /* e0 */
-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, /* f0 */
};
static char to_hex[] = "0123456789abcdef";

int git_oid_mkstr(git_oid *out, const char *str)
{
	size_t p;
	for (p = 0; p < sizeof(out->id); p++, str += 2) {
		int v = (from_hex[(unsigned char)str[0]] << 4)
		       | from_hex[(unsigned char)str[1]];
		if (v < 0)
			return GIT_ENOTOID;
		out->id[p] = (unsigned char)v;
	}
	return GIT_SUCCESS;
}

GIT_INLINE(char) *fmt_one(char *str, unsigned int val)
{
	*str++ = to_hex[val >> 4];
	*str++ = to_hex[val & 0xf];
	return str;
}

void git_oid_fmt(char *str, const git_oid *oid)
{
	size_t i;

	for (i = 0; i < sizeof(oid->id); i++)
		str = fmt_one(str, oid->id[i]);
}

void git_oid_pathfmt(char *str, const git_oid *oid)
{
	size_t i;

	str = fmt_one(str, oid->id[0]);
	*str++ = '/';
	for (i = 1; i < sizeof(oid->id); i++)
		str = fmt_one(str, oid->id[i]);
}

char *git_oid_allocfmt(const git_oid *oid)
{
	char *str = git__malloc(GIT_OID_HEXSZ + 1);
	if (!str)
		return NULL;
	git_oid_fmt(str, oid);
	str[GIT_OID_HEXSZ] = '\0';
	return str;
}

char *git_oid_to_string(char *out, size_t n, const git_oid *oid)
{
	char str[GIT_OID_HEXSZ];

	if (!out || n == 0 || !oid)
		return "";

	n--;  /* allow room for terminating NUL */

	if (n > 0) {
		git_oid_fmt(str, oid);
		if (n > GIT_OID_HEXSZ)
			n = GIT_OID_HEXSZ;
		memcpy(out, str, n);
	}

	out[n] = '\0';

	return out;
}

int git__parse_oid(git_oid *oid, char **buffer_out,
		const char *buffer_end, const char *header)
{
	const size_t sha_len = GIT_OID_HEXSZ;
	const size_t header_len = strlen(header);

	char *buffer = *buffer_out;

	if (buffer + (header_len + sha_len + 1) > buffer_end)
		return GIT_EOBJCORRUPTED;

	if (memcmp(buffer, header, header_len) != 0)
		return GIT_EOBJCORRUPTED;

	if (buffer[header_len + sha_len] != '\n')
		return GIT_EOBJCORRUPTED;

	if (git_oid_mkstr(oid, buffer + header_len) < GIT_SUCCESS)
		return GIT_EOBJCORRUPTED;

	*buffer_out = buffer + (header_len + sha_len + 1);

	return GIT_SUCCESS;
}

int git__write_oid(git_odb_source *src, const char *header, const git_oid *oid)
{
	char hex_oid[41];

	git_oid_fmt(hex_oid, oid);
	hex_oid[40] = 0;

	return git__source_printf(src, "%s %s\n", header, hex_oid);
}

void git_oid_mkraw(git_oid *out, const unsigned char *raw)
{
	memcpy(out->id, raw, sizeof(out->id));
}

void git_oid_cpy(git_oid *out, const git_oid *src)
{
	memcpy(out->id, src->id, sizeof(out->id));
}

int git_oid_cmp(const git_oid *a, const git_oid *b)
{
	return memcmp(a->id, b->id, sizeof(a->id));
}
