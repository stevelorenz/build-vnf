#! /bin/bash
# About: Profiling the memory usage with Valgrind for ./yolo_v2_pp.c
#
#

function compile() {
    make clean
    make -j 2 DEBUG=1 DPDK=0
}

if [[ $1 == "memcheck" ]]; then
    compile
    rm -f ./memcheck.log
    valgrind --leak-check=full --show-leak-kinds=all --log-file=memcheck.log ./yolo_v2_pp.out -m 1 -s 0 -n 1
elif [[ $1 == "heapuse" ]]; then
    compile
    rm -f ./heapuse.log
    rm -f ./massif.out.*
    valgrind --tool=massif --massif-out-file=massif.out.yolov2_tiny ./yolo_v2_pp.out -m 1 -s 0 -n 3 -c ./cfg/yolov2-tiny-voc.cfg
    valgrind --tool=massif --massif-out-file=massif.out.yolov2 ./yolo_v2_pp.out -m 1 -s 0 -n 3 -c ./cfg/yolov2.cfg
    valgrind --tool=massif --massif-out-file=massif.out.yolov2_f4 ./yolo_v2_pp.out -m 1 -s 0 -n 3 -c ./cfg/yolov2_f4.cfg
    valgrind --tool=massif --massif-out-file=massif.out.yolov2_f8 ./yolo_v2_pp.out -m 1 -s 0 -n 3 -c ./cfg/yolov2_f8.cfg
else
    echo "Invalid option!"
    echo "Usage: ./prof_mem.sh option"
    echo "  Option:"
    echo "    memcheck: Check memory errors via Valgrind memcheck."
    echo "    heapuse: Check heap memory usage via Valgrind massif."
fi
