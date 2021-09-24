#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <time.h>

#include <sys/time.h>

#include "ffpp/bpf_helpers_user.h" // NANOSEC_PER_SEC
#include "ffpp/general_helpers_user.h"
#include "ffpp/scaling_helpers_user.h"

void write_csv_file()
{
	int len;
	const char *file_dir = "/home";
	char save_path[1024] = "";
	len = snprintf(save_path, 1024, "%s/test-%d.csv", file_dir,
		       g_csv_num_round);
	if (len < 0) {
		fprintf(stderr, "Failed to generate csv save path.\n");
	}
	printf("File: %s\n", save_path);
	FILE *fptr;
	// name: test-"num".csv for each num
	fptr = fopen(save_path, "w+");
	if (fptr == NULL) {
		printf("File does not exist.\n");
		return;
	}
	unsigned int i;
	for (i = 0; i < g_csv_num_val; i++) {
		fprintf(fptr, "%f,%f,%f,%u\n", g_csv_ts[i], g_csv_pps[i],
			g_csv_cpu_util[i], g_csv_freq[i]);
	}
	fclose(fptr);
}

void write_csv_file_fb_in()
{
	int len;
	const char *file_dir = "/home";
	char save_path[1024] = "";
	// name: fb-"num".csv for each num
	len = snprintf(save_path, 1024, "%s/fb-%d.csv", file_dir,
		       g_csv_num_round);
	if (len < 0) {
		fprintf(stderr, "Failed to generate csv save path.\n");
	}
	printf("File: %s\n", save_path);
	FILE *fptr;
	fptr = fopen(save_path, "w+");
	if (fptr == NULL) {
		printf("File does not exist.\n");
		return;
	}
	unsigned int i;
	for (i = 0; i < g_csv_num_val; i++) {
		fprintf(fptr, "%f,%f,%f,%d,%d,%u\n", g_csv_ts[i],
			g_csv_in_pps[i], g_csv_out_pps[i], g_csv_out_delta[i],
			g_csv_offset[i], g_csv_freq[i]);
	}
	fclose(fptr);
}

void write_csv_file_tm()
{
	int len;
	const char *file_dir = "/home";
	char save_path[1024] = "";
	len = snprintf(save_path, 1024, "%s/tm-%d.csv", file_dir,
		       g_csv_num_round);
	if (len < 0) {
		fprintf(stderr, "Failed to generate csv save path.\n");
	}
	printf("File: %s\n", save_path);
	FILE *fptr;
	fptr = fopen(save_path, "w+");
	if (fptr == NULL) {
		printf("File does not exists.\n");
		return;
	}
	unsigned int i;
	for (i = 0; i < g_csv_num_val; i++) {
		fprintf(fptr, "%f,%f,%1.10f\n", g_csv_ts[i], g_csv_pps[i],
			g_csv_iat[i]);
	}
	fclose(fptr);
}

void write_csv_file_2vnf(int num_vnf)
{
	int len;
	const char *file_dir = "/home";
	char save_path[1024] = "";
	// name: vnf-"num_vnf"_test-"num".csv for each num
	len = snprintf(save_path, 1024, "%s/vnf-%d_test-%d.csv", file_dir,
		       num_vnf, g_csv_num_round);
	if (len < 0) {
		fprintf(stderr, "Failed to generate csv save path.\n");
	}
	printf("File: %s\n", save_path);
	FILE *fptr;
	fptr = fopen(save_path, "w+");
	if (fptr == NULL) {
		printf("File does not exist.\n");
		return;
	}
	unsigned int i;
	for (i = 0; i < g_csv_num_val; i++) {
		fprintf(fptr, "%f,%f,%f,%u\n", g_csv_ts[i],
			g_csv_pps_mult[num_vnf][i],
			g_csv_cpu_util_mult[num_vnf][i], g_csv_freq[i]);
	}
	fclose(fptr);
}

double get_time_of_day()
{
	struct timeval tv;
	gettimeofday(&tv, NULL); // same timestamp like turbostat
	cur_time = ((unsigned int)tv.tv_sec + (tv.tv_usec / 1e6));

	return cur_time;
}
