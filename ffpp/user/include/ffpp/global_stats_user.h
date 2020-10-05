/*
gloabls_stats_user.h
Global arrays for measurements
*/

#ifndef GLOBAL_STATS_USER_H
#define GLOBAL_STATS_USER_H

#define TOTAL_VALS 1000000 /* Array size */
#define NUM_VNFS 2 /* Max deployed CNF's */

extern double g_csv_pps[TOTAL_VALS]; /* ingress packet rate */
extern double g_csv_ts[TOTAL_VALS]; /* timestamp (same as used by turbostat) */
extern double g_csv_iat[TOTAL_VALS]; /* inter-arrival time */
extern double g_csv_cpu_util[TOTAL_VALS]; /* cpu utilization */
extern unsigned int g_csv_freq[TOTAL_VALS]; /* CPU frequency */
// Feedback
extern double g_csv_in_pps[TOTAL_VALS]; /* ingress pps */
extern double g_csv_out_pps[TOTAL_VALS]; /* egress pps */
extern int g_csv_out_delta[TOTAL_VALS]; /* packet delta between in and egress*/
extern int g_csv_offset[TOTAL_VALS]; /* packet offset between in and egress */
// Two VNfs
extern double g_csv_pps_mult[NUM_VNFS][TOTAL_VALS]; /* per CNF pps */
extern double g_csv_iat_mult[NUM_VNFS]
			    [TOTAL_VALS]; /* per CBF inter-arrival time */
extern double g_csv_cpu_util_mult[NUM_VNFS][TOTAL_VALS]; /* per CNF CPU util */
// Flags and counters
extern unsigned int g_csv_num_val; /* index counter */
extern int g_csv_num_round; /* test number */
extern int
	g_csv_empty_cnt; /* Count empty map readings (to detect when to store) */
extern int g_csv_empty_cnt_threshold; /* threshold for empty cnts */
extern bool
	g_csv_saved_stream; /* To not store multiple files during session gap */

extern double cur_time; /* timestamp */

#endif /* !GLOBAL_STATS_USER_H */
