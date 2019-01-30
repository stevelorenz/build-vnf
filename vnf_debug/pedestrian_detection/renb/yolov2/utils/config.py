import json

anchors = [[0.57273, 0.677385],
           [1.87446, 2.06253],
           [3.33843, 5.47434],
           [7.88282, 3.52778],
           [9.77052, 9.16828]]


def read_coco_category_id():
    f = open('./cocoapi/annotations/instances_val2017.json')
    ann = json.load(f)
    categories = ann['categories']
    class_names = []
    category_id_dict = {}
    for item in categories:
        category_id_dict[item['name']] = item['id']
        class_names.append(item['name'])
    return category_id_dict, class_names


category_id_dict, class_names = read_coco_category_id()
