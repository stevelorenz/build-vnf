#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <time.h>

#include <ffpp/scaling_helpers_user.h>
#include <ffpp/bpf_helpers_user.h> // NANOSEC_PER_SEC
#include <ffpp/general_helpers_user.h>

void write_csv_file()
{
	int len;
	const char *file_dir = "/home";
	char save_path[1024] = "";
	len = snprintf(save_path, 1024, "%s/test-%d.csv", file_dir,
		       g_csv_num_round);
	printf("File: %s\n", save_path);
	FILE *fptr;
	// name test-num.csv for each num
	fptr = fopen(save_path, "w+");
	if (fptr == NULL) {
		printf("File does not exist.\n");
		return;
	}
	int i;
	for (i = 0; i < g_csv_num_val; i++) {
		fprintf(fptr, "%f,%f,%f,%u\n", g_csv_ts[i], g_csv_pps[i],
			g_csv_cpu_util[i], g_csv_freq[i]);
	}
	fclose(fptr);
}

double get_time_of_day()
{
	struct timeval tv;
	gettimeofday(&tv, NULL);
	cur_time = ((unsigned int)tv.tv_sec + (tv.tv_usec / 1e6));
	// printf("Time %f\n", cur_time);

	return cur_time;
}
