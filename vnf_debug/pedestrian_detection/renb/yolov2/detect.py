import tensorflow as tf
from tensorflow.python.platform import gfile
import numpy as np
import os
import time
import statistics

from utils import *
from config import anchors, class_names
from dev import graph_editor as ge

def get_feature_map(orig, is_display=0, is_save=0):
    """
    orig: original feature map
    is_display: bool var for controlling display the feature map or not
    is_save: bool var for controlling save the feature map or not
    """
    feature_map = np.squeeze(orig, axis=0)
    m, n, c = feature_map.shape
    a = int(np.sqrt(c))
    while c % a is not 0:
        a = a - 1
    b = int(c / a)
    imgs = []
    cur = []

    for i in range(0, a):
        for j in range(0, b):
            tmp = np.zeros(shape=(orig.shape[0], orig.shape[1]))
            f_tmp = cv2.normalize(
                feature_map[:, :, i * j], tmp, 0, 255, cv2.NORM_MINMAX, cv2.CV_8UC1)
            cur.append(f_tmp)
        f_tmp = np.hstack(cur)
        imgs.append(f_tmp)
        cur = []
    res = np.vstack(imgs)
    if is_display:
        cv2.imshow("feature", res)
        cv2.waitKey(0)
        cv2.destroyAllWindows()
    if is_save:
        cv2.imwrite("./feature8.jpg", res)
    return feature_map


def eval_model(batch_size, sess):
    images_path = "pedes_images"
    imgs = [cv2.imread(os.path.join(images_path, i))
            for i in os.listdir(images_path)]

    indexes = [random.randint(0, len(imgs)) for i in range(batch_size)]
    # imgs_size = [imgs[index].shape for index in indexes]
    # generate batch
    input_size = (608, 608)
    img = preprocess_image(imgs[indexes[0]], input_size)
    print(type(img))
    for i in range(1, len(indexes)):
        img = np.append(img, preprocess_image(
            imgs[indexes[i]], input_size), axis=0)
    input_tensor = sess.graph.get_tensor_by_name("yolo/input:0")
    mid_tensor_8 = sess.graph.get_tensor_by_name("yolo/Pad_5:0")
    mid_tensor_9 = sess.graph.get_tensor_by_name("yolo/Pad_5:0")
    output_tensor = sess.graph.get_tensor_by_name("yolo/output:0")
    # feed into yolo
    t1 = []
    t2 = []
    for i in range(20):
        start = time.time()
        out1 = sess.run(mid_tensor_8, feed_dict={input_tensor: img})
        mid = time.time()
        out2 = sess.run(output_tensor, feed_dict={mid_tensor_9: out1})
        end = time.time()
        sess.run(output_tensor, feed_dict={input_tensor: img})
        cur = time.time()
        total = cur - end
        t1.append((mid - start)/batch_size)
        t2.append((end - mid)/batch_size)
        print(mid - start, end - mid, total)
    return statistics.mean(t1[2:]), statistics.mean(t2[2:]), statistics.stdev(t1[2:]), statistics.stdev(t2[2:])

    """
    output_sizes = input_size[0]//32, input_size[1]//32
    output_decoded = decode(model_output=output_tensor, output_sizes=output_sizes,
                            num_class=len(class_names), anchors=anchors)
    start = time.time()
    bboxes, obj_probs, class_probs = sess.run(
        output_decoded, feed_dict={input_tensor: img})
    end = time.time()
    print("inference time for batch_size {} is {} seconds".format(
        batch_size, end-start))

    res_boxes = []
    res_scores = []
    res_class_max_index = []
    res_imgs = []
    for i in range(batch_size):
        bboxes_cur, scores_cur, class_max_index_cur = postprocess(
            bboxes[i], obj_probs[i], class_probs[i], image_shape=imgs_size[i])
        res_boxes.append(bboxes_cur)
        res_scores.append(scores_cur)
        res_class_max_index.append(class_max_index_cur)
        img_detection = draw_detection(
            imgs[indexes[i]], bboxes_cur, scores_cur, class_max_index_cur, class_names)
        res_imgs.append(img_detection)
    is_save = 1
    if is_save:
        for i, img in enumerate(res_imgs):
            cv2.imwrite("./tmp/{}.jpg".format(i), res_imgs[i])
    """


def main():
    input_size = (608, 608)
    sess = ge.read_model('./model/yolo.pb')
    # img_orig = cv2.imread('./data/car.jpg')
    

    input_tensor = sess.graph.get_tensor_by_name("yolo/input:0")
    output_tensor = sess.graph.get_tensor_by_name("yolo/output:0")
    for i in range(5):
        img_orig = cv2.imread('./pedes_images/01-20170320211734-25.jpg')
        img = preprocess_image(img_orig)
        # get feature map and if possible, display it
        # layers[num]: num is the num'th layer, e.g, layers[2] is the second layer, namely the first maxpool layer
        
        # feature_map = sess.run(sess.graph.get_tensor_by_name(
        #     "yolo/Pad_5:0"), feed_dict={input_tensor: img})
        # get_feature_map(feature_map, 0, 0)
        
        output_sizes = input_size[0]//32, input_size[1]//32
        output_decoded = decode(model_output=output_tensor, output_sizes=output_sizes,
                                num_class=len(class_names), anchors=anchors)
        start = time.time()
        bboxes, obj_probs, class_probs = sess.run(
            output_decoded, feed_dict={input_tensor: img})
        end = time.time()
        bboxes, scores, class_max_index = postprocess(
            bboxes, obj_probs, class_probs, image_shape=img_orig.shape[:2])
        
        img_detection = draw_detection(
            img_orig, bboxes, scores, class_max_index, class_names)
        # cv2.imwrite("./data/detection.jpg", img_detection)
        
        print('YOLO_v2 detection has done! spent {} seconds'.format(end - start))
        cv2.imshow("detection_results", img_detection)
        cv2.waitKey(0)


if __name__ == "__main__":
    main()
    # sess = ge.read_model('./model/yolo.pb')
    # eval_model(1, sess)
