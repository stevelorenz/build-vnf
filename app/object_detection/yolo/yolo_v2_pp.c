/*
 *
 * About: Test preprocessing possibilities for YOLO v2 object detection model
 *
 * MARK : This code is used to evaluate the performance of using YOLO as a
 * virtualized application, there is not packet handling included. The
 * corresponded VNF should be placed in ../../vnf_debug/
 *
 * Email: xianglinks@gmail.com
 *
 */

#include <math.h>
#include <sched.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#include <rte_bus.h>
#include <rte_common.h>
#include <rte_compat.h>
#include <rte_config.h>
#include <rte_cycles.h>
#include <rte_memory.h>
#include <rte_pci_dev_feature_defs.h>
#include <rte_per_lcore.h>

#include <linux/limits.h>

#include <darknet.h>

#define MAX_FILENAME_SIZE 50
#define MAX_FRAME_NUM 200

/* TODO:  <19-01-19, Zuo> Check the common value for these threshold in papers
 */
#define DETECT_THRESH 0.25
#define DETECT_HIER_THRESH 0.5

/*****************
 *  Workarounds  *
 *****************/

typedef struct node {
	void *val;
	struct node *next;
	struct node *prev;
} node;

typedef struct list {
	int size;
	node *front;
	node *back;
} list;

/***********
 *  Utils  *
 ***********/

void save_delay_csv(int img_idx, int *type_arr, double *delay_arr, int n,
		    char *path)
{
	size_t i;
	FILE *fd;

	fd = fopen(path, "a+");
	if (fd == NULL) {
		perror("Can not open the CSV file!");
	}
	for (i = 0; i < n; ++i) {
		fprintf(fd, "%d,%lu,%d,%f\n", img_idx, i, type_arr[i],
			delay_arr[i]);
	}
	fclose(fd);
}

__attribute__((inline)) void forward_network_delay(network *net,
						   network_state state,
						   double *layer_delay_arr,
						   int *layer_type_arr)
{
	uint64_t last_tsc;
	double delay;

	state.workspace = net->workspace;
	int i = 0;
	int j = 0;
	for (i = 0; i < net->n; ++i) {
		last_tsc = rte_get_tsc_cycles();
		state.index = i;
		layer l = net->layers[i];
		// BUG: scal_cpu causes segmentation fault
		// if (l.delta) {
		//        scal_cpu(l.outputs * l.batch, 0, l.delta, 1);
		//}
		l.forward(l, state);
		delay = (1.0 / rte_get_timer_hz()) * 1000.0 *
			(rte_get_tsc_cycles() - last_tsc);
		layer_delay_arr[i] = delay;
		layer_type_arr[i] = l.type;

		state.input = l.output;
	}
}

/********************
 *  Test Functions  *
 ********************/

/**
 * @brief Compare different YOLO models
 * Performance parameter:
 *      - Output size of each layer
 *      - Inference delay of each layer
 */
void perf_delay_layerwise(char *model, int image_st, unsigned int image_num,
			  unsigned int save_output, unsigned int layer1_io_file)
{
	uint64_t last_tsc;
	double delay; // Delay in milliseconds
	size_t i;
	size_t img_idx;
	char *datacfg;
	char *cfgfile;
	char *weightfile;
	char input_file[MAX_FILENAME_SIZE];
	char output_file[MAX_FILENAME_SIZE];

	float nms = 0.45;

	if (strcmp(model, "yolov2-tiny") == 0) {
		datacfg = "/root/darknet/cfg/voc.data";
		cfgfile = "/root/darknet/cfg/yolov2-tiny-voc.cfg";
		weightfile = "/root/darknet/yolov2-tiny-voc.weights";
	} else if (strcmp(model, "yolov2") == 0) {
		datacfg = "/root/darknet/cfg/coco.data";
		cfgfile = "/root/darknet/cfg/yolov2.cfg";
		weightfile = "/root/darknet/yolov2.weights";
	} else {
		fprintf(stderr, "Unknown model name.\n");
		exit(1);
	}

	list *options = read_data_cfg(datacfg);
	char *name_list = option_find_str(options, "names", "data/names.list");
	char **names = get_labels(name_list);
	image **alphabet = load_alphabet();

	network *net = load_network_custom(cfgfile, weightfile, 1, 1);
	fuse_conv_batchnorm(*net);
	calculate_binary_weights(*net);

	snprintf(output_file, MAX_FILENAME_SIZE, "%s_%s.csv", "net_info",
		 model);
	FILE *net_info = fopen(output_file, "w+");

	for (i = 0; i < net->n; ++i) {
		layer l = net->layers[i];
		fprintf(net_info, "%lu, %u,%d,%d\n", i, l.type, l.inputs,
			l.outputs);
	}
	fclose(net_info);

	double layer_delay_arr[net->n];
	int layer_type_arr[net->n];

	for (img_idx = image_st; img_idx < image_st + image_num; ++img_idx) {
		printf("Current image index: %lu\n", img_idx);
		sprintf(input_file, "../../dataset/pedestrian_walking/%lu.jpg",
			img_idx);
		image im = load_image_color(input_file, 0, 0);
		/*image sized = letterbox_image(im, net->w, net->h);*/
		image sized = resize_image(im, net->w, net->h);

		layer l = net->layers[net->n - 1];
		float *X = sized.data;
		last_tsc = rte_get_tsc_cycles();

		network_state state;
		state.net = *net;
		state.index = 0;
		state.input = X;
		state.train = 0;
		state.truth = 0;
		state.delta = 0;

		forward_network_delay(net, state, layer_delay_arr,
				      layer_type_arr);

		snprintf(output_file, MAX_FILENAME_SIZE, "%s_%s.csv",
			 "per_layer_delay", model);
		save_delay_csv(img_idx, layer_type_arr, layer_delay_arr, net->n,
			       output_file);

		delay = (1.0 / rte_get_timer_hz()) * 1000.0 *
			(rte_get_tsc_cycles() - last_tsc);
		printf("Predicted in %lf milli-seconds.\n", delay);

		if (unlikely(save_output == 1)) {
			int nboxes = 0;
			detection *dets = get_network_boxes(net, im.w, im.h,
							    DETECT_THRESH,
							    DETECT_HIER_THRESH,
							    0, 1, &nboxes, 0);
			do_nms_sort(dets, nboxes, l.classes, nms);
			draw_detections_v3(im, dets, nboxes, DETECT_THRESH,
					   names, alphabet, l.classes, 1);
			snprintf(output_file, MAX_FILENAME_SIZE, "%lu_%s",
				 img_idx, model);
			save_image(im, output_file);
			free_detections(dets, nboxes);
		}
		free_image(im);
		free_image(sized);
	}
}

__attribute__((always_inline)) void infer_frame(size_t img_idx, image *frame,
						network *net,
						unsigned int save_output)
{
	char output_file[MAX_FILENAME_SIZE];

	/*image sized = letterbox_image(*frame, net->w, net->h);*/
	image sized = resize_image(*frame, net->w, net->h);

	network_predict(*net, sized.data);
	if (save_output == 1) {
		layer l = net->layers[net->n - 1];
		snprintf(output_file, MAX_FILENAME_SIZE, "./%lu_output.bin",
			 img_idx);
		FILE *fd = fopen(output_file, "w+");
		/* MARK: In darknet layer struct, the type of pointer of image
     * struct is float */
		fwrite(l.output, sizeof(float), l.outputs, fd);
		fclose(fd);
	}
}

/**
 * @brief Test YOLO v2 preprocessing delay
 *        The preprocessing is to perform the first X conv and pooling layers
 *
 * @mark: This function is profiled with Valgrind Memcheck and Massif for memory
 *        management. The goal is to reduce the memory usage.
 *
 * @todo: Only use accelerated darknet after sufficient stability tests
 */
void perf_yolov2_pp(char *cfgfile, int image_st, unsigned int image_num,
		    unsigned int save_output, unsigned int keep_run,
		    char *csv_prefix, unsigned int dpdk_enabled)
{
	uint64_t begin_tsc = 0;
	uint64_t last_tsc = 0;
	size_t img_idx = 0;
	double delay = 0.0;
	char *weightfile = "/root/darknet/yolov2.weights";
	char input_file[MAX_FILENAME_SIZE];
	char output_file[MAX_FILENAME_SIZE];
	double delay_arr[MAX_FRAME_NUM] = { 0 };
	size_t delay_idx = 0;
	FILE *fd = NULL;

	// ISSUE: Eating hundreds of MB when loading layers (even only conv and
	// maxpooling layers).
	printf("To be loaded cfg file: %s\n", cfgfile);

	network *net = load_network_custom(cfgfile, weightfile, 1, 1);
	fuse_conv_batchnorm(*net);
	calculate_binary_weights(*net);
	srand(22222222);

	if (dpdk_enabled == 1) {
		begin_tsc = rte_get_tsc_cycles();
	}

	for (img_idx = image_st; img_idx < image_st + image_num; ++img_idx) {
		snprintf(input_file, MAX_FILENAME_SIZE,
			 "../../dataset/pedestrian_walking/%lu.jpg", img_idx);
		image im = load_image_color(input_file, 0, 0);

		if (likely(dpdk_enabled == 1)) {
			last_tsc = rte_get_tsc_cycles();
			infer_frame(img_idx, &im, net, save_output);
			delay = (1.0 / rte_get_timer_hz()) * 1000.0 *
				(rte_get_tsc_cycles() - last_tsc);
			delay_arr[delay_idx] = delay;
			delay_idx += 1;
		} else {
			infer_frame(img_idx, &im, net, save_output);
		}

		if (keep_run == 1 && img_idx == (image_st + image_num - 1)) {
			img_idx = image_st;
		}
	}

	if (keep_run == 0) {
		delay = (1.0 / rte_get_timer_hz()) * 1000.0 *
			(rte_get_tsc_cycles() - begin_tsc);

		snprintf(output_file, MAX_FILENAME_SIZE,
			 "./%s_yolo_v2_pp_delay.csv", csv_prefix);
		fd = fopen(output_file, "w+");
		if (fd == NULL) {
			perror("Can not open CSV file to store processing "
			       "delay.\n");
			exit(1);
		}
		for (delay_idx = 0; delay_idx < image_num; ++delay_idx) {
			fprintf(fd, "%f\n", delay_arr[delay_idx]);
		}
		fprintf(fd, "total:%f\n", delay);
		fclose(fd);
	}
}

void save_tmp_output(float *arr, uint16_t len)
{
	FILE *fd = NULL;
	uint16_t i = 0;

	fd = fopen("./tmp_output.csv", "w+");
	for (i = 0; i < len; ++i) {
		fprintf(fd, "%f, ", arr[i]);
		if (i != 0 && i % 76 == 0) {
			fprintf(fd, "\n");
		}
	}
	fclose(fd);
}

/**
 * @brief main
 *        DPDK libraries is used HERE for CPU cycles counting and
 * runtime monitoring, not used for packet IO.
 */
int main(int argc, char *argv[])
{
	int ret;
	int opt;
	int test_mode = 0;
	int image_st = 0;
	int image_num = 1;
	char csv_prefix[10] = "";
	char cfgfile[PATH_MAX] = "./cfg/yolov2_f4.cfg";

	unsigned int dpdk_enabled = 0;
	unsigned int save_output = 0;
	unsigned int keep_run = 0;

	printf("Test YOLO v2 !\n");

#ifdef DPDK
	dpdk_enabled = 1;
	printf("DPDK is enabled.\n");
	ret = rte_eal_init(argc, argv);
	if (ret < 0) {
		rte_exit(EXIT_FAILURE, "Invalid EAL arguments.\n");
	}
#endif

	/*Parse additional args*/
	while ((opt = getopt(argc, argv, "akos:m:n:p:c:")) != -1) {
		switch (opt) {
		case 'm':
			test_mode = atoi(optarg);
			break;
		case 'o':
			printf("Save output images into current "
			       "directory.\n");
			save_output = 1;
			break;
		case 'k':
			printf("Keep running. Loop over given input images.\n");
			keep_run = 1;
			break;
		case 's':
			image_st = atoi(optarg);
			break;
		case 'n':
			image_num = atoi(optarg);
			break;
		case 'p':
			strncpy(csv_prefix, optarg, 10);
			break;
		case 'c':
			strncpy(cfgfile, optarg, PATH_MAX);
			break;
		default:
			break;
		}
	}
	if (image_num == 0) {
		fprintf(stderr, "Invalid image number!\n");
	}
	printf("The image start index:%d, image number:%d\n", image_st,
	       image_num);

	if (dpdk_enabled == 1) {
		uint64_t freq;
		freq = rte_get_tsc_hz();
		printf("The frequency of RDTSC counter is %lu \n", freq);
	}

	fclose(stderr);
	stderr = fopen("./stderr_log.log", "a+");

	if (test_mode == 0) {
#ifndef DPDK
		fprintf(stderr, "DPDK must be enabled for test_mode 0!\n");
		exit(1);
#endif
		printf("Run layerwise delay evaluation of yolov2 and "
		       "yolov2-tiny.\n");
		perf_delay_layerwise("yolov2", image_st, image_num, save_output,
				     0);
		perf_delay_layerwise("yolov2-tiny", image_st, image_num,
				     save_output, 0);
	} else if (test_mode == 1) {
		printf("Run YOLO pre-processing test.\n");
		perf_yolov2_pp(cfgfile, image_st, image_num, save_output,
			       keep_run, csv_prefix, dpdk_enabled);
	} else {
		fprintf(stderr, "Invalid test mode!\n");
	}

	printf("Program exits.\n");
	return 0;
}
