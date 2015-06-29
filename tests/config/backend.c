#include "clar_libgit2.h"
#include "git2/sys/config.h"

void test_config_backend__checks_version(void)
{
	git_config *cfg;
	git_config_backend backend = GIT_CONFIG_BACKEND_INIT;

	backend.version = 1024;

	cl_git_pass(git_config_new(&cfg));
	cl_git_fail(git_config_add_backend(cfg, &backend, 0, false));

	giterr_clear();
	backend.version = 1024;
	cl_git_fail(git_config_add_backend(cfg, &backend, 0, false));

	git_config_free(cfg);
}
