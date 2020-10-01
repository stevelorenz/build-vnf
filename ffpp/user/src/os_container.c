/*
 * os_container.c
 */

#include <stdio.h>
#include <unistd.h>

#include <rte_eal.h>

#include <ffpp/os_container.h>

static const char *CGROUP_CPU_PREFIX = "/sys/fs/cgroup/cpu/docker";

int ffpp_docker_update_cpu_rsc(const char *container_id,
			       struct ffpp_cpu_rsc *rsc)
{
	int ret;
	char path_buf[PATH_MAX];
	ret = snprintf(path_buf, PATH_MAX, "%s/%s/cpu.cfs_quota_us",
		       CGROUP_CPU_PREFIX, container_id);
	if (ret < 0) {
		fprintf(stderr, "Failed to create path name.\n");
		return -1;
	}
	if (access(path_buf, W_OK) == -1) {
		fprintf(stderr, "The path: %s is not writable.\n", path_buf);
		return -1;
	}

	FILE *f;
	f = fopen(path_buf, "w");
	fwrite(&(rsc->cfs_quota_us), sizeof(rsc->cfs_quota_us), 1, f);
	printf("%d\n", ret);
	fclose(f);

	return 0;
}
