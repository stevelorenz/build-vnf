#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# vim:fenc=utf-8

"""
About: Read installed Conda packages with sorted size.
"""

import json
import os
import pathlib
import pprint

prefix = os.environ["CONDA_PREFIX"]
meta_data_path = pathlib.Path(prefix) / "conda-meta"
meta_data_files = meta_data_path.glob("*.json")

meta_data = []
for meta_data_file in meta_data_files:
    name = meta_data_file.name.split("-")[0]
    with open(meta_data_file, "r", encoding="utf-8") as f:
        d = json.load(f)
        size = d["size"]
    meta_data.append((name, size))

meta_data.sort(key=lambda x: x[1])
meta_data.reverse()
pprint.pprint(meta_data)
