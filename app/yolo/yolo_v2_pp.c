/*
 * yolo_v2_pp.c
 *
 * About: Frame preprocessing for YOLO v2 object detection model
 * Email: xianglinks@gmail.com
 *
 */

#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <rte_common.h>
#include <rte_compat.h>
#include <rte_cycles.h>
#include <rte_memory.h>

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

/**
 * @brief Test YOLO v2
 */
void perf_delay_layerwise()
{
        uint64_t last_tsc;
        double delay; // Delay in milliseconds

        float thresh = 0.5;
        char* filename
            = "../../dataset/pedestrian_walking/01-20170320211734-00.jpg";
        char* outfile = "./output";
        char* datacfg = "/root/darknet/cfg/coco.data";
        char* cfgfile = "/root/darknet/cfg/yolov2.cfg";
        char* weightfile = "/root/darknet/yolov2.weights";

        list* options = read_data_cfg(datacfg);
        char* name_list = option_find_str(options, "names", "data/names.list");
        char** names = get_labels(name_list);
        image** alphabet = load_alphabet();

        last_tsc = rte_get_tsc_cycles();
        network* net = load_network(cfgfile, weightfile, 0);
        delay = (1.0 / rte_get_timer_hz()) * 1000.0
            * (rte_get_tsc_cycles() - last_tsc);
        ll_insert_delay("load_network", delay);

        set_batch_network(net, 1);
        srand(time(0));

        char buff[256];
        char* input = buff;
        float nms = 0.45;

        strncpy(input, filename, 256);
        last_tsc = rte_get_tsc_cycles();
        image im = load_image_color(input, 0, 0);
        image sized = letterbox_image(im, net->w, net->h);
        delay = (1.0 / rte_get_timer_hz()) * 1000.0
            * (rte_get_tsc_cycles() - last_tsc);
        ll_insert_delay("load_image", delay);

        layer l = net->layers[net->n - 1];

        float* X = sized.data;
        last_tsc = rte_get_tsc_cycles();
        /* TODO: Split network_predict into layer wise */
        network_predict(net, X);
        delay = (1.0 / rte_get_timer_hz()) * 1000.0
            * (rte_get_tsc_cycles() - last_tsc);
        ll_insert_delay("network_predict", delay);

        int nboxes = 0;
        detection* dets
            = get_network_boxes(net, im.w, im.h, 0.5, 0.5, 0, 1, &nboxes);
        do_nms_sort(dets, nboxes, l.classes, nms);
        draw_detections(im, dets, nboxes, 0.5, names, alphabet, l.classes);
        free_detections(dets, nboxes);
        save_image(im, outfile);
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
        printf("The frequency of RDTSC counter is %ld \n", freq);

        perf_delay_layerwise();

        ll_cleanup();

        printf("Program exits.\n");
        return 0;
}
