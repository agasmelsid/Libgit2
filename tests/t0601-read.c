#include "test_lib.h"
#include "test_helpers.h"
#include "index.h"

#include <git2/odb.h>
#include <git2/index.h>

#define TEST_INDEX_PATH "../resources/testrepo.git/index"
#define TEST_INDEX2_PATH "../resources/gitgit.index"

#define TEST_INDEX_ENTRY_COUNT 109
#define TEST_INDEX2_ENTRY_COUNT 1437

struct test_entry {
	unsigned int index;
	char path[128];
	size_t file_size;
	time_t mtime;
};

struct test_entry TEST_ENTRIES[] = {
	{4, "Makefile", 5064, 0x4C3F7F33},
	{62, "tests/Makefile", 2631, 0x4C3F7F33},
	{36, "src/index.c", 10014, 0x4C43368D},
	{6, "git.git-authors", 2709, 0x4C3F7F33},
	{48, "src/revobject.h", 1448, 0x4C3F7FE2}
};

BEGIN_TEST(index_loadempty_test)
	git_index *index;

	must_pass(git_index_open_bare(&index, "in-memory-index"));
	must_be_true(index->on_disk == 0);

	must_pass(git_index_read(index));

	must_be_true(index->on_disk == 0);
	must_be_true(git_index_entrycount(index) == 0);
	must_be_true(index->sorted);

	git_index_free(index);
END_TEST

BEGIN_TEST(index_load_test)
	git_index *index;
	unsigned int i;
	git_index_entry **entries;

	must_pass(git_index_open_bare(&index, TEST_INDEX_PATH));
	must_be_true(index->on_disk);

	must_pass(git_index_read(index));

	must_be_true(index->on_disk);
	must_be_true(git_index_entrycount(index) == TEST_INDEX_ENTRY_COUNT);
	must_be_true(index->sorted);

	entries = (git_index_entry **)index->entries.contents;

	for (i = 0; i < ARRAY_SIZE(TEST_ENTRIES); ++i) {
		git_index_entry *e = entries[TEST_ENTRIES[i].index];

		must_be_true(strcmp(e->path, TEST_ENTRIES[i].path) == 0);
		must_be_true(e->mtime.seconds == TEST_ENTRIES[i].mtime);
		must_be_true(e->file_size == TEST_ENTRIES[i].file_size);
	}

	git_index_free(index);
END_TEST

BEGIN_TEST(index2_load_test)
	git_index *index;

	must_pass(git_index_open_bare(&index, TEST_INDEX2_PATH));
	must_be_true(index->on_disk);

	must_pass(git_index_read(index));

	must_be_true(index->on_disk);
	must_be_true(git_index_entrycount(index) == TEST_INDEX2_ENTRY_COUNT);
	must_be_true(index->sorted);
	must_be_true(index->tree != NULL);

	git_index_free(index);
END_TEST

BEGIN_TEST(index_find_test)
	git_index *index;
	unsigned int i;

	must_pass(git_index_open_bare(&index, TEST_INDEX_PATH));
	must_pass(git_index_read(index));

	for (i = 0; i < ARRAY_SIZE(TEST_ENTRIES); ++i) {
		int idx = git_index_find(index, TEST_ENTRIES[i].path);
		must_be_true((unsigned int)idx == TEST_ENTRIES[i].index);
	}

	git_index_free(index);
END_TEST

BEGIN_TEST(index_findempty_test)
	git_index *index;
	unsigned int i;

	must_pass(git_index_open_bare(&index, "fake-index"));

	for (i = 0; i < ARRAY_SIZE(TEST_ENTRIES); ++i) {
		int idx = git_index_find(index, TEST_ENTRIES[i].path);
		must_be_true(idx == GIT_ENOTFOUND);
	}

	git_index_free(index);
END_TEST
