import tensorflow as tf
from tensorflow.python.platform import gfile
import numpy as np
import os
import time
import json

from detect import *
from utils import *
from config import anchors, class_names
from dev import graph_editor as ge


def gen_results(imgs_path):
    """
    detect object of all images and generate the results format according to coco submission
    """
    # init model
    sess = ge.read_model('./model/yolo.pb')
    input_tensor = sess.graph.get_tensor_by_name("input:0")
    tmp = sess.graph.get_tensor_by_name("Pad_5:0")
    output_tensor = sess.graph.get_tensor_by_name("output:0")
    # read images
    base = imgs_path
    img_names = sorted(os.listdir(imgs_path))
    img_paths = [os.path.join(base, img_name) for img_name in img_names]
    # generate results
    keys = ["image_id", "category_id", "bbox", "score"]
    results = []
    input_size = (608, 608)
    cnt = 0
    for img_name in img_paths:
        img_id = int(img_name.split("/")[-1].split(".")[0])
        img_orig = cv2.imread(img_name)
        
        img = preprocess_image(img_orig)
        output_sizes = input_size[0]//32, input_size[1]//32
        tmp_data = sess.run(tmp, feed_dict={input_tensor:img})
        output_decoded = decode(model_output=output_tensor, output_sizes=output_sizes,
                                num_class=len(class_names), anchors=anchors)
        bboxes, obj_probs, class_probs = sess.run(
            output_decoded, feed_dict={tmp: tmp_data})

        bboxes, scores, class_max_index = postprocess(
            bboxes, obj_probs, class_probs, image_shape=img_orig.shape[:2])
        for i in range(len(bboxes)):
            res = {}
            res[keys[0]] = img_id
            res[keys[1]] = int(class_max_index[i])
            res[keys[2]] = [int(j) for j in [bboxes[i][0], bboxes[i][1], bboxes[i]
                            [2] - bboxes[i][0], bboxes[i][3] - bboxes[i][1]]]
            res[keys[3]] = round(float(scores[i]), 3)
            results.append(res)
        print("img: {} finished".format(img_name))
        cnt += 1
        if cnt == 500:
            break
    return results


if __name__ == "__main__":

    results = gen_results('./cocoapi/images/val2017')
    print(results)
    with open('./cocoapi/results/results.json', 'w') as outfile:
        json.dump(results, outfile)

