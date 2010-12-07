#include "test_lib.h"
#include "test_helpers.h"
#include "commit.h"

#include <git2/odb.h>
#include <git2/commit.h>
#include <git2/tag.h>

static const char *tag1_id = "b25fa35b38051e4ae45d4222e795f9df2e43f1d1";
static const char *tag2_id = "7b4384978d2493e851f9cca7858815fac9b10980";
static const char *tagged_commit = "e90810b8df3e80c413d903f631643c716887138d";

BEGIN_TEST(readtag)
	git_repository *repo;
	git_tag *tag1, *tag2;
	git_commit *commit;
	git_oid id1, id2, id_commit;

	must_pass(git_repository_open(&repo, REPOSITORY_FOLDER));

	git_oid_mkstr(&id1, tag1_id);
	git_oid_mkstr(&id2, tag2_id);
	git_oid_mkstr(&id_commit, tagged_commit);

	must_pass(git_tag_lookup(&tag1, repo, &id1));

	must_be_true(strcmp(git_tag_name(tag1), "test") == 0);
	must_be_true(git_tag_type(tag1) == GIT_OBJ_TAG);

	tag2 = (git_tag *)git_tag_target(tag1);
	must_be_true(tag2 != NULL);

	must_be_true(git_oid_cmp(&id2, git_tag_id(tag2)) == 0);

	commit = (git_commit *)git_tag_target(tag2);
	must_be_true(commit != NULL);

	must_be_true(git_oid_cmp(&id_commit, git_commit_id(commit)) == 0);

	git_repository_free(repo);
END_TEST
