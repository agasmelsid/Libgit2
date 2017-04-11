#include "clar_libgit2.h"
#include "git2/sys/repository.h"

#include "patch.h"
#include "repository.h"
#include "buf_text.h"

#include "../patch/patch_common.h"

static git_repository *repo = NULL;

void test_apply_partial__initialize(void)
{
	repo = cl_git_sandbox_init("renames");
}

void test_apply_partial__cleanup(void)
{
	cl_git_sandbox_cleanup();
}

static int skip_addition(
	const git_diff_delta *delta,
	const git_diff_hunk *hunk,
	void *payload)
{
	GIT_UNUSED(delta);
	GIT_UNUSED(payload);

	return (hunk->new_lines > hunk->old_lines) ? GIT_PATCH_SKIP : GIT_PATCH_APPLY;
}

static int skip_deletion(
	const git_diff_delta *delta,
	const git_diff_hunk *hunk,
	void *payload)
{
	GIT_UNUSED(delta);
	GIT_UNUSED(payload);

	return (hunk->new_lines < hunk->old_lines) ? GIT_PATCH_SKIP : GIT_PATCH_APPLY;
}

static int skip_change(
	const git_diff_delta *delta,
	const git_diff_hunk *hunk,
	void *payload)
{
	GIT_UNUSED(delta);
	GIT_UNUSED(payload);

	return (hunk->new_lines == hunk->old_lines) ? GIT_PATCH_SKIP : GIT_PATCH_APPLY;
}

static int apply_buf(
	const char *old,
	const char *oldname,
	const char *new,
	const char *newname,
	const char *expected,
	const git_diff_options *diff_opts,
	git_diff_hunk_cb hunk_cb,
	void *payload)
{
	git_patch *patch;
	git_buf result = GIT_BUF_INIT;
	git_buf patchbuf = GIT_BUF_INIT;
	char *filename;
	unsigned int mode;
	int error;
	size_t oldsize = strlen(old);
	size_t newsize = strlen(new);

	cl_git_pass(git_patch_from_buffers(&patch, old, oldsize, oldname, new, newsize, newname, diff_opts));
	if ((error = git_patch_apply(&result, &filename, &mode, patch, old, oldsize, hunk_cb, payload)) == 0) {
		cl_assert_equal_s(expected, result.ptr);
		cl_assert_equal_s(newname, filename);
		cl_assert_equal_i(0100644, mode);
	}

	git__free(filename);
	git_buf_free(&result);
	git_buf_free(&patchbuf);
	git_patch_free(patch);

	return error;
}

void test_apply_partial__prepend_and_change_skip_addition(void)
{
	cl_git_pass(apply_buf(
		FILE_ORIGINAL, "file.txt",
		FILE_PREPEND_AND_CHANGE, "file.txt",
		FILE_ORIGINAL, NULL, skip_addition, NULL));
}

void test_apply_partial__prepend_and_change_nocontext_skip_addition(void)
{
	git_diff_options diff_opts = GIT_DIFF_OPTIONS_INIT;
	diff_opts.context_lines = 0;

	cl_git_pass(apply_buf(
		FILE_ORIGINAL, "file.txt",
		FILE_PREPEND_AND_CHANGE, "file.txt",
		FILE_CHANGE_MIDDLE, &diff_opts, skip_addition, NULL));
}

void test_apply_partial__prepend_and_change_skip_change(void)
{
	cl_git_pass(apply_buf(
		FILE_ORIGINAL, "file.txt",
		FILE_PREPEND_AND_CHANGE, "file.txt",
		FILE_PREPEND_AND_CHANGE, NULL, skip_change, NULL));
}

void test_apply_partial__prepend_and_change_nocontext_skip_change(void)
{
	git_diff_options diff_opts = GIT_DIFF_OPTIONS_INIT;
	diff_opts.context_lines = 0;

	cl_git_pass(apply_buf(
		FILE_ORIGINAL, "file.txt",
		FILE_PREPEND_AND_CHANGE, "file.txt",
		FILE_PREPEND, &diff_opts, skip_change, NULL));
}

void test_apply_partial__delete_and_change_skip_deletion(void)
{
	cl_git_pass(apply_buf(
		FILE_ORIGINAL, "file.txt",
		FILE_DELETE_AND_CHANGE, "file.txt",
		FILE_ORIGINAL, NULL, skip_deletion, NULL));
}

void test_apply_partial__delete_and_change_nocontext_skip_deletion(void)
{
	git_diff_options diff_opts = GIT_DIFF_OPTIONS_INIT;
	diff_opts.context_lines = 0;

	cl_git_pass(apply_buf(
		FILE_ORIGINAL, "file.txt",
		FILE_DELETE_AND_CHANGE, "file.txt",
		FILE_CHANGE_MIDDLE, &diff_opts, skip_deletion, NULL));
}

void test_apply_partial__delete_and_change_skip_change(void)
{
	cl_git_pass(apply_buf(
		FILE_ORIGINAL, "file.txt",
		FILE_DELETE_AND_CHANGE, "file.txt",
		FILE_DELETE_AND_CHANGE, NULL, skip_change, NULL));
}

void test_apply_partial__delete_and_change_nocontext_skip_change(void)
{
	git_diff_options diff_opts = GIT_DIFF_OPTIONS_INIT;
	diff_opts.context_lines = 0;

	cl_git_pass(apply_buf(
		FILE_ORIGINAL, "file.txt",
		FILE_DELETE_AND_CHANGE, "file.txt",
		FILE_DELETE_FIRSTLINE, &diff_opts, skip_change, NULL));
}
