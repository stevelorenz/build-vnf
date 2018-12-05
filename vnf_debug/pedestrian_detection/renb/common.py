import cv2


def resize_image(in_image, new_width, new_height, out_image=None, resize_mode=cv2.INTER_CUBIC):
    '''

    :param in_image: 输入的图片
    :param new_width: resize后的新图片的宽
    :param new_height: resize后的新图片的长
    :param out_image: 保存resize后的新图片的地址
    :param resize_mode: 用于resize的cv2中的模式
    :return: resize后的新图片
    '''
    img = cv2.resize(in_image, (new_width, new_height), resize_mode)
    if out_image:
        cv2.imwrite(out_image, img)
    return img


def clip_pic(img, rect):
    '''

    :param img: 输入的图片
    :param rect: rect矩形框的4个参数
    :return: 输入的图片中相对应rect位置的部分 与 矩形框的一对对角点和长宽信息
    '''
    x, y, w, h = rect[0], rect[1], rect[2], rect[3]
    x_1 = x + w
    y_1 = y + h
    return img[y:y_1, x:x_1, :], [x, y, x_1, y_1, w, h]


def IOU(ver1, vertice2):
    '''
    用于计算两个矩形框的IOU
    :param ver1: 第一个矩形框
    :param vertice2: 第二个矩形框
    :return: 两个矩形框的IOU值
    '''
    def if_intersection(xmin_a, xmax_a, ymin_a, ymax_a, xmin_b, xmax_b, ymin_b, ymax_b):
        if_intersect = False
        if (xmin_a < xmax_b) and (xmax_a > xmin_b) and (ymin_a < ymax_b) and (ymax_a > ymin_b):
            if_intersect = True
        else:
            return if_intersect

        if if_intersect:
            x_sorted_list = sorted([xmin_a, xmax_a, xmin_b, xmax_b])
            y_sorted_list = sorted([ymin_a, ymax_a, ymin_b, ymax_b])
            x_intersect_w = x_sorted_list[2] - x_sorted_list[1]
            y_intersect_h = y_sorted_list[2] - y_sorted_list[1]
            area_inter = x_intersect_w * y_intersect_h
            return area_inter

    vertice1 = [ver1[0], ver1[1], ver1[0]+ver1[2], ver1[1]+ver1[3]]
    area_inter = if_intersection(vertice1[0], vertice1[2], vertice1[1],
                                 vertice1[3], vertice2[0], vertice2[2], vertice2[1], vertice2[3])
    if area_inter:
        area_1 = ver1[2] * ver1[3]
        area_2 = vertice2[4] * vertice2[5]
        iou = float(area_inter) / (area_1 + area_2 - area_inter)
        return iou
    return False
