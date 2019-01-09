/*
 * yolo_v2_pp.c
 *
 * About: Test preprocessing possibilities for YOLO v2 object detection model
 *
 * MARK : Codes is ONLY used for testing the functionality, so without
 * performance and code optimization.
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

#include <darknet.h>

#define MAX_FILENAME_SIZE 50

/*****************
 *  Linked List  *
 *****************/

struct delay_entry {
        char* label;
        double delay;
        struct delay_entry* next;
};

struct delay_entry* head = NULL;
struct delay_entry* current = NULL;

void ll_insert_delay(char* label, double delay)
{
        struct delay_entry* link
            = (struct delay_entry*)malloc(sizeof(struct delay_entry));
        link->label = strdup(label);
        link->delay = delay;
        link->next = head;
        head = link;
}

void ll_print_delay()
{
        struct delay_entry* p = head;
        printf("Print all stored delays: \n");
        while (p != NULL) {
                printf("%s => %f ms,", p->label, p->delay);
                p = p->next;
        }
        printf("\n");
}

void ll_cleanup()
{
        printf("Cleanup delay list.\n");
        struct delay_entry* p = head;
        while (p != NULL) {
                free(p);
                p = p->next;
        }
}

void save_delay_csv(
    int img_idx, int* type_arr, double* delay_arr, int n, char* path)
{
        size_t i;
        FILE* fd;

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

/**
 * @brief Test YOLO v2 model
 *
 * @param model
 * @param image_st
 * @param image_num
 * @param save_output
 * @param layer1_io_file:
 */
void perf_delay_layerwise(char* model, int image_st, unsigned int image_num,
    unsigned int save_output, unsigned int layer1_io_file)
{
        uint64_t last_tsc;
        double delay; // Delay in milliseconds
        size_t i;
        size_t img_idx;
        char* datacfg;
        char* cfgfile;
        char* weightfile;
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

        list* options = read_data_cfg(datacfg);
        char* name_list = option_find_str(options, "names", "data/names.list");
        char** names = get_labels(name_list);
        image** alphabet = load_alphabet();

        network* net = load_network(cfgfile, weightfile, 0);
        last_tsc = rte_get_tsc_cycles();
        delay = (1.0 / rte_get_timer_hz()) * 1000.0
            * (rte_get_tsc_cycles() - last_tsc);
        ll_insert_delay("load_network", delay);

        snprintf(
            output_file, MAX_FILENAME_SIZE, "%s_%s.csv", "net_info", model);
        FILE* net_info = fopen(output_file, "w+");

        for (i = 0; i < net->n; ++i) {
                layer l = net->layers[i];
                fprintf(net_info, "%lu, %u,%d,%d\n", i, l.type, l.inputs,
                    l.outputs);
        }
        fclose(net_info);

        set_batch_network(net, 1);
        srand(time(0));

        double layer_delay_arr[net->n];
        int layer_type_arr[net->n];
        for (img_idx = image_st; img_idx < image_st + image_num; ++img_idx) {
                printf("Current image index: %lu\n", img_idx);
                sprintf(input_file, "../../dataset/pedestrian_walking/%lu.jpg",
                    img_idx);
                image im = load_image_color(input_file, 0, 0);
                image sized = letterbox_image(im, net->w, net->h);

                layer l = net->layers[net->n - 1];
                float* X = sized.data;
                last_tsc = rte_get_tsc_cycles();

                network orig = *net;
                net->input = X;
                net->truth = 0;
                net->train = 0;
                net->delta = 0;

                if (layer1_io_file) {
                        printf("[TEST] The IO data of the first layer is "
                               "stored in files.\n");
                }

                /* MARK: Iterate over layers in the network */
                network net_cur = *net;
                FILE* io_fd = NULL;

                for (i = 0; i < net_cur.n; ++i) {
                        net_cur.index = i;
                        layer l = net_cur.layers[i];
                        last_tsc = rte_get_tsc_cycles();
                        l.forward(l, net_cur);
                        if (i == 1 && layer1_io_file == 1) {
                                snprintf(output_file, MAX_FILENAME_SIZE,
                                    "layer1_%s_input.bin", model);
                                io_fd = fopen(output_file, "w+");
                                fwrite(net_cur.input, 1, l.inputs, io_fd);
                                fclose(io_fd);
                        }
                        delay = (1.0 / rte_get_timer_hz()) * 1000.0
                            * (rte_get_tsc_cycles() - last_tsc);
                        layer_delay_arr[i] = delay;
                        layer_type_arr[i] = l.type;
                        net_cur.input = l.output;

                        if (i == 1 && layer1_io_file == 1) {
                                snprintf(output_file, MAX_FILENAME_SIZE,
                                    "layer1_%s_output.bin", model);
                                io_fd = fopen(output_file, "w+");
                                fwrite(l.output, 1, l.outputs, io_fd);
                                fclose(io_fd);
                        }
                }

                snprintf(output_file, MAX_FILENAME_SIZE, "%s_%s.csv",
                    "per_layer_delay", model);
                save_delay_csv(img_idx, layer_type_arr, layer_delay_arr, net->n,
                    output_file);
                *net = orig;

                if (unlikely(save_output == 1)) {
                        int nboxes = 0;
                        detection* dets = get_network_boxes(
                            net, im.w, im.h, 0.5, 0.5, 0, 1, &nboxes);
                        do_nms_sort(dets, nboxes, l.classes, nms);
                        draw_detections(
                            im, dets, nboxes, 0.5, names, alphabet, l.classes);
                        free_detections(dets, nboxes);
                        snprintf(output_file, MAX_FILENAME_SIZE, "%lu_%s",
                            img_idx, model);
                        save_image(im, output_file);
                }
        }

        ll_print_delay();
}

/**
 * @brief Test YOLO v2 preprocessing delay
 * Only run layers including conv1, pool1, conv2 and pool2
 */
void perf_yolov2_pp(
    int image_st, unsigned int image_num, unsigned int save_output)
{
        uint64_t last_tsc;
        size_t img_idx = 0;
        double delay;
        char* cfgfile = "/app/yolo/cfg/yolov2_pp.cfg";
        char* weightfile = "/root/darknet/yolov2.weights";
        char input_file[MAX_FILENAME_SIZE];
        char output_file[MAX_FILENAME_SIZE];
        double delay_arr[200];
        size_t delay_idx = 0;

        network* net = load_network(cfgfile, weightfile, 0);

        set_batch_network(net, 1);
        srand(time(0));

        for (img_idx = image_st; img_idx < image_st + image_num; ++img_idx) {
                snprintf(input_file, MAX_FILENAME_SIZE,
                    "../../dataset/pedestrian_walking/%lu.jpg", img_idx);
                image im = load_image_color(input_file, 0, 0);

                last_tsc = rte_get_tsc_cycles();
                image sized = letterbox_image(im, net->w, net->h);
                network_predict(net, sized.data);
                delay = (1.0 / rte_get_timer_hz()) * 1000.0
                    * (rte_get_tsc_cycles() - last_tsc);
                delay_arr[delay_idx] = delay;
                delay_idx += 1;
                if (save_output == 1) {
                        layer l = net->layers[net->n - 1];
                        snprintf(output_file, MAX_FILENAME_SIZE,
                            "./%lu_output.bin", img_idx);
                        FILE* fd = fopen(output_file, "a+");
                        fwrite(l.output, 1, l.outputs, fd);
                        fclose(fd);
                }
        }
        snprintf(output_file, MAX_FILENAME_SIZE, "./%d_yolo_v2_pp_delay.csv",
            getpid());
        FILE* fd = fopen(output_file, "w+");
        for (delay_idx = 0; delay_idx < image_num; ++delay_idx) {
                fprintf(fd, "%f\n", delay_arr[delay_idx]);
        }
        fclose(fd);
}

/**
 * @brief main
 *        DPDK libraries is used HERE for CPU cycles counting and
 * runtime monitoring, not used for packet IO.
 */
int main(int argc, char* argv[])
{
        int ret;
        int opt;
        int test_mode = 0;
        int image_st = 0;
        int image_num = 0;
        int save_output = 0;

        printf("Test YOLO v2 !\n");
        ret = rte_eal_init(argc, argv);
        if (ret < 0) {
                rte_exit(EXIT_FAILURE, "Invalid EAL arguments.\n");
        }
        /*Parse additional args*/
        while ((opt = getopt(argc, argv, "os:m:n:")) != -1) {
                switch (opt) {
                case 'm':
                        test_mode = atoi(optarg);
                        break;
                case 'o':
                        printf("Save output images into current "
                               "directory.\n");
                        save_output = 1;
                        break;
                case 's':
                        image_st = atoi(optarg);
                        break;
                case 'n':
                        image_num = atoi(optarg);
                        break;
                default:
                        break;
                }
        }
        if (image_num == 0) {
                fprintf(stderr, "Invalid image number!\n");
        }
        printf(
            "The image start index:%d, image number:%d\n", image_st, image_num);

        uint64_t freq;
        freq = rte_get_tsc_hz();
        printf("The frequency of RDTSC counter is %lu \n", freq);

        fclose(stderr);
        stderr = fopen("./stderr_log.log", "a+");

        if (test_mode == 0) {
                printf("Run layerwise delay evaluation of yolov2 and "
                       "yolov2-tiny.\n");
                perf_delay_layerwise(
                    "yolov2", image_st, image_num, save_output, 0);
                perf_delay_layerwise(
                    "yolov2-tiny", image_st, image_num, save_output, 0);
        }
        if (test_mode == 1) {
                printf("Run YOLO pre-processing test.\n");
                perf_yolov2_pp(image_st, image_num, save_output);
        }

        ll_cleanup();
        printf("Program exits.\n");
        return 0;
}
