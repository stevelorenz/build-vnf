#! /bin/bash

echo "Build darknet image with ../../dockerfiles/Dockerfile.darknet ..."
sudo docker build -t darknet --file ../../dockerfiles/Dockerfile.darknet .
