/*
 * Copyright (C) the libgit2 contributors. All rights reserved.
 *
 * This file is part of libgit2, distributed under the GNU GPL v2 with
 * a Linking Exception. For full terms see the included COPYING file.
 */
#ifndef INCLUDE_git_describe_h__
#define INCLUDE_git_describe_h__

#include "common.h"
#include "types.h"
#include "buffer.h"

/**
 * @file git2/describe.h
 * @brief Git describing routines
 * @defgroup git_describe Git describing routines
 * @ingroup Git
 * @{
 */
GIT_BEGIN_DECL

typedef enum {
	GIT_DESCRIBE_DEFAULT,
	GIT_DESCRIBE_TAGS,
	GIT_DESCRIBE_ALL,
} git_describe_strategy_t;

/**
 * Describe options structure
 *
 * Zero out for defaults.  Initialize with `GIT_DESCRIBE_OPTIONS_INIT` macro to
 * correctly set the `version` field.  E.g.
 *
 *		git_describe_opts opts = GIT_DESCRIBE_OPTIONS_INIT;
 */
typedef struct git_describe_opts {
	unsigned int version;

	unsigned int max_candidates_tags; /** default: 10 */
	unsigned int abbreviated_size;
	unsigned int describe_strategy; /** default: GIT_DESCRIBE_DEFAULT */
	const char *pattern;
	int always_use_long_format;
	int only_follow_first_parent;
	int show_commit_oid_as_fallback;
} git_describe_opts;

#define GIT_DESCRIBE_DEFAULT_MAX_CANDIDATES_TAGS 10
#define GIT_DESCRIBE_DEFAULT_ABBREVIATED_SIZE 7

#define GIT_DESCRIBE_OPTIONS_VERSION 1
#define GIT_DESCRIBE_OPTIONS_INIT { \
	GIT_DESCRIBE_OPTIONS_VERSION, \
	GIT_DESCRIBE_DEFAULT_MAX_CANDIDATES_TAGS, \
	GIT_DESCRIBE_DEFAULT_ABBREVIATED_SIZE}

GIT_EXTERN(int) git_describe_object(
	git_buf *out,
	git_object *committish,
	git_describe_opts *opts);

/** @} */
GIT_END_DECL

#endif
