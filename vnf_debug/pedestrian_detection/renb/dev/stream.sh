#!/bin/sh
ffmpeg -re -i test.MOV -an -vcodec mjpeg -huffman default -qscale 1 -f rtp rtp://127.0.0.1:8888/test.sdp
