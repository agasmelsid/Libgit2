#include "test_lib.h"
#include "test_helpers.h"
#include "commit.h"
#include "revwalk.h"
#include <git2/odb.h>
#include <git2/commit.h>

BEGIN_TEST(list_timesort_test)

	git_revwalk_list list;
	git_revwalk_listnode *n;
	int i, t;
	time_t previous_time;

#define TEST_SORTED() \
		previous_time = INT_MAX;\
	for (n = list.head; n != NULL; n = n->next) {\
		must_be_true(n->walk_commit->commit_object->commit_time <= previous_time);\
		previous_time = n->walk_commit->commit_object->commit_time;\
	}

#define CLEAR_LIST() \
	for (n = list.head; n != NULL; n = n->next) {\
		free(n->walk_commit->commit_object);\
		free(n->walk_commit);\
	}\
	git_revwalk_list_clear(&list);

	memset(&list, 0x0, sizeof(git_revwalk_list));
	srand((unsigned int)time(NULL));

	for (t = 0; t < 20; ++t) {
		const int test_size = rand() % 500 + 500;

		/* Purely random sorting test */
		for (i = 0; i < test_size; ++i) {
			git_commit *c = git__malloc(sizeof(git_commit));
			git_revwalk_commit *rc = git__malloc(sizeof(git_revwalk_commit));

			c->commit_time = (time_t)rand();
			rc->commit_object = c;

			git_revwalk_list_push_back(&list, rc);
		}

		git_revwalk_list_timesort(&list);
		TEST_SORTED();
		CLEAR_LIST();
	}

	/* Try to sort list with all dates equal. */
	for (i = 0; i < 200; ++i) {
		git_commit *c = git__malloc(sizeof(git_commit));
		git_revwalk_commit *rc = git__malloc(sizeof(git_revwalk_commit));

		c->commit_time = 0;
		rc->commit_object = c;

		git_revwalk_list_push_back(&list, rc);
	}

	git_revwalk_list_timesort(&list);
	TEST_SORTED();
	CLEAR_LIST();

	/* Try to sort empty list */
	git_revwalk_list_timesort(&list);
	TEST_SORTED();

END_TEST
