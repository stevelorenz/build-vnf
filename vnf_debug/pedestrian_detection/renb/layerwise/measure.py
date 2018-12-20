import tensorflow as tf
import os
import numpy as np
import net
import weights_loader
import cv2
import time

# resolve model from ckpt files


def resolve_model(model_path):
    # use one cpu core
    # session_conf = tf.ConfigProto(
    #     intra_op_parallelism_threads=1,
    #     inter_op_parallelism_threads=1)
    # sess = tf.Session(config=session_conf)
    meta_path = os.path.join(model_path, "model.ckpt.meta")
    ckpt_path = model_path

    # use all cores
    sess = tf.Session()
    saver = tf.train.import_meta_graph(meta_path)
    saver.restore(sess, tf.train.latest_checkpoint(ckpt_path))

    return sess


def preprocessing(input_img_path, input_height, input_width):

    input_image = cv2.imread(input_img_path)

    # Resize the image and convert to array of float32
    resized_image = cv2.resize(
        input_image, (input_height, input_width), interpolation=cv2.INTER_CUBIC)
    image_data = np.array(resized_image, dtype='f')

    # Normalization [0,255] -> [0,1]
    image_data /= 255.

    # BGR -> RGB? The results do not change much
    # copied_image = image_data
    #image_data[:,:,2] = copied_image[:,:,0]
    #image_data[:,:,0] = copied_image[:,:,2]

    # Add the dimension relative to the batch size needed for the input placeholder "x"
    image_array = np.expand_dims(image_data, 0)  # Add batch dimension

    return image_array


def inference(sess, preprocessed_image, layer_output):
    feed_dict = {net.x: preprocessed_image}
    sess.run(layer_output, feed_dict=feed_dict)


if __name__ == "__main__":
    # the weights file can be found in https://pjreddie.com/media/files/yolov2-tiny-voc.weights
    weights_path = './dev/yolov2-tiny-voc.weights'
    input_img_path = './dev/person.jpg'
    output_image_path = './dev/output.jpg'

    # if ckpt does not exist, it will create ckpt files automatically after the fisrt runnung of this program
    # model can also be resolved from ckpt files, see func: resolve_model above
    # when getting tensors, e.g o8, a name of layer should be assigned in net.py, eg:
    # o8 = tf.nn.conv2d(..., name='o8'), then call tf.get_default_graph().get_tensor_by_name('o8')
    ckpt_folder_path = './dev/ckpt/'

    input_height = 416
    input_width = 416
    score_threshold = 0.3
    iou_threshold = 0.3

    # Definition of the session
    # using one cpu core
    session_conf = tf.ConfigProto(
        intra_op_parallelism_threads=1,
        inter_op_parallelism_threads=1)
    sess = tf.InteractiveSession(config=session_conf)
    # use all cores
    # sess = tf.InteractiveSession()
    tf.global_variables_initializer().run()

    # Check for an existing checkpoint and load the weights (if it exists) or do it from binary file
    print('Looking for a checkpoint...')
    saver = tf.train.Saver()
    _ = weights_loader.load(sess, weights_path, ckpt_folder_path, saver)

    # Preprocess the input image
    print('Preprocessing...')
    preprocessed_image = preprocessing(
        input_img_path, input_height, input_width)

    t = []
    layers = [net.o1, net.o2, net.o3, net.o4,
              net.o5, net.o6, net.o7, net.o8, net.o9]
    shapes = []
    for i in range(9):
        c = []
        for j in range(10):
            start = time.time()
            inference(sess, preprocessed_image, layers[i])
            end = time.time()
            c.append(end-start)
        shapes.append(layers[i].get_shape().as_list())
        t.append(sum(c)/10)

    params = net.params

    for i in range(9):
        print("inference time of conv_{}: {}, nums of output_{}: {}, nums params of layer_{}: {}".format(
            i+1, t[i] - t[i-1] if i != 0 else t[i], i+1, shapes[i][1] *
            shapes[i][2] * shapes[i][3], i+1,  params[i] -
            params[i-1] if i != 0 else params[0]
        ))
