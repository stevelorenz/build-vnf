/*
 * yolo_v2_pp.c
 *
 * About: Frame preprocessing for YOLO v2 object detection model
 * Email: xianglinks@gmail.com
 *
 */

#include <darknet.h>
#include <math.h>
#include <time.h>

#include <rte_common.h>
#include <rte_cycles.h>

/**
 * @brief Test YOLO v2
 */
void test_yolo_v2()
{
        float thresh = 0.5;
        char* filename
            = "../../dataset/pedestrian_walking/01-20170320211734-00.jpg";
        char* outfile = "./output.jpg";
        char* datacfg = "/root/darknet/cfg/coco.data";
        char* cfgfile = "/root/darknet/cfg/yolov2.cfg";
        char* weightfile = "/root/darknet/yolov2.weights";

        list* options = read_data_cfg(datacfg);
        char* name_list = option_find_str(options, "names", "data/names.list");
        char** names = get_labels(name_list);

        /*network* net = load_network(cfgfile, weightfile, 0);*/
}

int main(int argc, char* argv[])
{
        printf("Test YOLO v2 !\n");
        uint64_t freq;
        freq = rte_get_tsc_hz();
        printf("The frequency of RDTSC counter is %ld \n", freq);

        test_yolo_v2();
        return 0;
}
