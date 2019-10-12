#! /bin/bash

echo "Stop and remove all containers"
sudo docker stop $(sudo docker ps -aq)
sudo docker rm $(sudo docker ps -aq)

echo "Remove all output images"
rm ./*.jpg
rm ./*.jpeg
