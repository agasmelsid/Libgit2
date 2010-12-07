#include "test_lib.h"
#include "test_helpers.h"
#include "commit.h"

#include <git2/odb.h>
#include <git2/commit.h>
#include <git2/revwalk.h>

static const char *tree_oid = "1810dff58d8a660512d4832e740f692884338ccd";

BEGIN_TEST(tree_in_memory_add_test)
	const unsigned int entry_count = 128;

	git_repository *repo;
	git_tree *tree;
	unsigned int i;
	git_oid entry_id;

	must_pass(git_repository_open(&repo, REPOSITORY_FOLDER));
	must_pass(git_tree_new(&tree, repo));

	git_oid_mkstr(&entry_id, tree_oid);
	for (i = 0; i < entry_count; ++i) {
		char filename[32];
		sprintf(filename, "file%d.txt", i);
		must_pass(git_tree_add_entry(tree, &entry_id, filename, 040000));
	}

	must_be_true(git_tree_entrycount(tree) == entry_count);
	must_pass(git_object_write((git_object *)tree));
	must_pass(remove_loose_object(REPOSITORY_FOLDER, (git_object *)tree));

	git_object_free((git_object *)tree);

	git_repository_free(repo);
END_TEST

BEGIN_TEST(tree_add_entry_test)
	git_oid id;
	git_repository *repo;
	git_tree *tree;
	git_tree_entry *entry;
	unsigned int i;
	/* char hex_oid[41]; */

	must_pass(git_repository_open(&repo, REPOSITORY_FOLDER));

	git_oid_mkstr(&id, tree_oid);

	must_pass(git_tree_lookup(&tree, repo, &id));

	must_be_true(git_tree_entrycount(tree) == 3);

	git_tree_add_entry(tree, &id, "zzz_test_entry.dat", 0);
	git_tree_add_entry(tree, &id, "01_test_entry.txt", 0);

	must_be_true(git_tree_entrycount(tree) == 5);

	entry = git_tree_entry_byindex(tree, 0);
	must_be_true(strcmp(git_tree_entry_name(entry), "01_test_entry.txt") == 0);

	entry = git_tree_entry_byindex(tree, 4);
	must_be_true(strcmp(git_tree_entry_name(entry), "zzz_test_entry.dat") == 0);

	must_pass(git_tree_remove_entry_byname(tree, "README"));
	must_be_true(git_tree_entrycount(tree) == 4);

	for (i = 0; i < git_tree_entrycount(tree); ++i) {
		entry = git_tree_entry_byindex(tree, i);
		must_be_true(strcmp(git_tree_entry_name(entry), "README") != 0);
	}

	must_pass(git_object_write((git_object *)tree));

/*
	git_oid_fmt(hex_oid, git_tree_id(tree));
	hex_oid[40] = 0;
	printf("TREE New SHA1: %s\n", hex_oid);
*/

	must_pass(remove_loose_object(REPOSITORY_FOLDER, (git_object *)tree));
	git_object_free((git_object *)tree);
	git_repository_free(repo);
END_TEST
