import tensorflow as tf
from tensorflow.python.platform import gfile
import numpy as np
import os
import time
import json

from detect import *
from utils.imgutils import *
from utils.config import anchors, class_names, category_id_dict
from dev import graph_editor as ge
from dev import compressor


def gen_results(imgs_path):
    """
    detect object of all images and generate the results format according to coco submission
    """
    
    # init model
    sess1 = ge.read_model('./model/splitted_models/part1.pb', "part1")
    sess2 = ge.read_model('./model/splitted_models/yolo.pb', "yolo")
    tensor_names = [t.name for op in sess1.graph.get_operations()
                    for t in op.values()]
    input1 = sess1.graph.get_tensor_by_name("part1/input:0")
    output1 = sess1.graph.get_tensor_by_name("part1/Pad_5:0")

    input2 = sess2.graph.get_tensor_by_name("yolo/Pad_5:0")
    output2 = sess2.graph.get_tensor_by_name("yolo/output:0")

    compressor_obj = compressor.Compressor()
    # read images
    base = imgs_path
    img_names = sorted(os.listdir(imgs_path))
    img_paths = [os.path.join(base, img_name) for img_name in img_names]
    # generate results
    keys = ["image_id", "category_id", "bbox", "score"]
    results = []
    input_size = (608, 608)
    for img_name in img_paths:
        
        img_id = int(img_name.split("/")[-1].split(".")[0])
        img_orig = cv2.imread(img_name)
        
        img = preprocess_image(img_orig)
        output_sizes = input_size[0]//32, input_size[1]//32
        featuremap = sess1.run(output1, feed_dict={input1:img})
        flag = compressor_obj.fill_buffer(featuremap)
        if flag is not 0:
            raise "Error"
        decompressed_data = compressor_obj.read_buffer()
        model_output = sess2.run(output2, feed_dict={input2: decompressed_data})
        bboxes, obj_probs, class_probs = decode_result(model_output=model_output, output_sizes=output_sizes,
                                num_class=len(class_names), anchors=anchors)
        bboxes, scores, class_max_index = postprocess(
            bboxes, obj_probs, class_probs, image_shape=img_orig.shape[:2])
        img_detection = draw_detection(
            img_orig, bboxes, scores, class_max_index, class_names)
        # cv2.imshow("detection_results", img_detection)
        # cv2.waitKey(0)
        # cv2.destroyAllWindows()
        for i in range(len(bboxes)):
            res = {}
            res[keys[0]] = img_id
            res[keys[1]] = int(category_id_dict[class_names[class_max_index[i]]])
            res[keys[2]] = [int(j) for j in [bboxes[i][0], bboxes[i][1], bboxes[i]
                            [2] - bboxes[i][0], bboxes[i][3] - bboxes[i][1]]]
            res[keys[3]] = round(float(scores[i]), 3)
            results.append(res)
        print("img: {} finished".format(img_name))
    return results


if __name__ == "__main__":

    results = gen_results('./cocoapi/images/val2017')
    with open('./cocoapi/results/results_orig.json', 'w') as outfile:
        json.dump(results, outfile)

