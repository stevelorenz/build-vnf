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

#include <rte_bus.h>
#include <rte_common.h>
#include <rte_compat.h>
#include <rte_config.h>
#include <rte_cycles.h>
#include <rte_memory.h>
#include <rte_pci_dev_feature_defs.h>
#include <rte_per_lcore.h>

#include <darknet.h>

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

void save_delay_csv(int img_idx, double* arr, int n, char* path)
{
        size_t i;
        FILE* fd;

        fd = fopen(path, "a+");
        for (i = 0; i < n; ++i) {
                fprintf(fd, "%d,%lu,%f\n", img_idx, i, arr[i]);
        }
        fclose(fd);
}

/**
 * @brief Test YOLO v2
 */
void perf_delay_layerwise(int save_output)
{
        uint64_t last_tsc;
        double delay; // Delay in milliseconds
        size_t i;
        size_t img_idx;

        /*MARK: For YOLO Tiny model*/
        char* datacfg = "/root/darknet/cfg/voc.data";
        char* cfgfile = "/root/darknet/cfg/yolov2-tiny-voc.cfg";
        char* weightfile = "/root/darknet/yolov2-tiny-voc.weights";

        /*char* datacfg = "/root/darknet/cfg/coco.data";*/
        /*char* cfgfile = "/root/darknet/cfg/yolov2.cfg";*/
        /*char* weightfile = "/root/darknet/yolov2.weights";*/

        list* options = read_data_cfg(datacfg);
        char* name_list = option_find_str(options, "names", "data/names.list");
        char** names = get_labels(name_list);
        image** alphabet = load_alphabet();

        last_tsc = rte_get_tsc_cycles();
        network* net = load_network(cfgfile, weightfile, 0);
        delay = (1.0 / rte_get_timer_hz()) * 1000.0
            * (rte_get_tsc_cycles() - last_tsc);
        ll_insert_delay("load_network", delay);

        FILE* net_info = fopen("./net_info.log", "w+");
        for (i = 0; i < net->n; ++i) {
                layer l = net->layers[i];
                fprintf(net_info, "%lu, %u,%d,%d\n", i, l.type, l.inputs,
                    l.outputs);
        }
        fclose(net_info);

        set_batch_network(net, 1);
        srand(time(0));

        char buff[256];
        char* input = buff;
        char* output = buff;

        float nms = 0.45;
        double layer_delay_arr[net->n];
        uint16_t image_num = 116;
        uint16_t image_st = 19;
        /*char* output_fn = NULL;*/

        for (img_idx = image_st; img_idx < image_num; ++img_idx) {
                printf("Current image index: %lu\n", img_idx);
                sprintf(
                    input, "../../dataset/pedestrian_walking/%lu.jpg", img_idx);
                image im = load_image_color(input, 0, 0);
                image sized = letterbox_image(im, net->w, net->h);

                layer l = net->layers[net->n - 1];

                float* X = sized.data;
                last_tsc = rte_get_tsc_cycles();

                network orig = *net;
                net->input = X;
                net->truth = 0;
                net->train = 0;
                net->delta = 0;

                /* MARK: Iterate over layers in the network */
                network net_cur = *net;
                for (i = 0; i < net_cur.n; ++i) {
                        net_cur.index = i;
                        layer l = net_cur.layers[i];
                        last_tsc = rte_get_tsc_cycles();
                        l.forward(l, net_cur);
                        delay = (1.0 / rte_get_timer_hz()) * 1000.0
                            * (rte_get_tsc_cycles() - last_tsc);
                        layer_delay_arr[i] = delay;
                        net_cur.input = l.output;
                }
                save_delay_csv(
                    img_idx, layer_delay_arr, net->n, "./per_layer_delay.csv");
                *net = orig;

                if (unlikely(save_output == 1)) {
                        int nboxes = 0;
                        detection* dets = get_network_boxes(
                            net, im.w, im.h, 0.5, 0.5, 0, 1, &nboxes);
                        do_nms_sort(dets, nboxes, l.classes, nms);
                        draw_detections(
                            im, dets, nboxes, 0.5, names, alphabet, l.classes);
                        free_detections(dets, nboxes);
                        sprintf(output, "%lu", img_idx);
                        save_image(im, output);
                }
        }

        ll_print_delay();
}

/**
 * @brief main
 *        DPDK libraries is used HERE for CPU cycles counting and runtime
 *        monitoring, not used for packet IO.
 */
int main(int argc, char* argv[])
{
        int ret;
        printf("Test YOLO v2 !\n");
        ret = rte_eal_init(argc, argv);
        if (ret < 0) {
                rte_exit(EXIT_FAILURE, "Invalid EAL arguments.\n");
        }

        uint64_t freq;
        freq = rte_get_tsc_hz();
        printf("The frequency of RDTSC counter is %lu \n", freq);

        fclose(stderr);
        stderr = fopen("./stderr_log.log", "a+");
        perf_delay_layerwise(1);

        ll_cleanup();

        printf("Program exits.\n");
        return 0;
}
