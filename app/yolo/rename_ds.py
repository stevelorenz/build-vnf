#! /usr/bin/env python3
# -*- coding: utf-8 -*-
# vim:fenc=utf-8

import os

PATH = "../../dataset/pedestrian_walking/"
IMAGE_ARR = sorted(os.listdir(PATH))
for i, image in enumerate(IMAGE_ARR):
    os.rename(
        os.path.join(PATH, image),
        os.path.join(PATH, str(i) + ".jpg")
    )
