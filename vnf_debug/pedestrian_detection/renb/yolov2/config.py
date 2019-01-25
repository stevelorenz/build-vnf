anchors = [[0.57273, 0.677385],
           [1.87446, 2.06253],
           [3.33843, 5.47434],
           [7.88282, 3.52778],
           [9.77052, 9.16828]]

def read_coco_labels():
    f = open("./meta/coco_classes.txt")
    class_names = []
    for l in f.readlines():
        l = l.strip()
        class_names.append(l)
    return class_names

class_names = read_coco_labels()