#!/usr/bin/python

import time
import cv2
import time
import numpy as np
from yolo import detector


class ObjInfo:

    def __init__(self):
        self.max_info_buffer_size = 3
        # info_buffer[
        #   item: (framenumber, timestamp, {obj1: [cls,[x1,y1,x2,y2]], obj2: [cls,[x1,y1,x2,y2]], ...})
        # ]
        self.info_buffer = []
        self.width = 432
        self.height = 320
        # self.yolo = self.init_model('./yolo/ckpt')

    def init_model(self, model_path):
        yolo = detector.YoloDetector(model_path)
        return yolo

    def update_info(self, info):
        # info: (timestamp_1, frame_number, prediction_result)
        old_time = info[0]
        new_time = time.time()
        frame_number = info[1]
        prediction_result = info[2]
        results = {}
        for i in range(len(prediction_result)):
            results[i] = [prediction_result[i][2], prediction_result[i][0]]
        info_formatted = (frame_number, old_time, results)
        self.info_buffer.append(info_formatted)
        if len(self.info_buffer) > self.max_info_buffer_size:
            self.info_buffer = self.info_buffer[-self.max_info_buffer_size:]

    def calc_objs_speed(self, info_buffer):
        """
        calculate the speed of every objects
        """
        # result: {o1: [cls, center1, speed1], o2: [cls, center2, speed2]}
        result = {}
        result_tmp = {}
        timestamps = []
        for i, info in enumerate(info_buffer):
            timestamps.append(info[1])
            obj_loc = info[2]
            for id in range(len(obj_loc)):
                label = obj_loc[id][0]
                center = ((obj_loc[id][1][0] + obj_loc[id][1][2]) / 2,
                          (obj_loc[id][1][1] + obj_loc[id][1][3]) / 2)
                if id not in result_tmp.keys():
                    result_tmp[id] = []
                result_tmp[id].append([label, center])
        for obj in result_tmp.keys():
            x = result_tmp[obj]
            loc_delta = [x[n][1][0] - x[n - 1][1][0] for n in range(1, len(x))]
            time_delta = [timestamps[n] - timestamps[n-1]
                          for n in range(1, len(timestamps))]
            speed = loc_delta[1] / time_delta[1]
            result[obj] = [x[-1][0], x[-1][1], speed]
        return result


a = ObjInfo()
# info_buffer[
#   item: (framenumber, timestamp, {obj1: [cls,[x1,y1,x2,y2]], obj2: [cls,[x1,y1,x2,y2]], ...})
# ]
info_buffer = [
    (1, 100, {0: ["p", [1, 1, 2, 2]], 1: ["q", [3, 3, 4, 4]]}),
    (2, 101, {0: ["p", [3, 1, 4, 2]], 1: ["q", [4, 3, 5, 4]]}),
    (3, 102, {0: ["p", [5, 1, 6, 2]], 1: ["q", [5, 3, 6, 4]]})
]
start = time.time()
result = a.calc_objs_speed(info_buffer)
end = time.time()
# print result and calculation time
print(result, end-start)
