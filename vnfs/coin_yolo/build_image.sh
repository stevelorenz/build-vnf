#!/bin/bash

docker build -f ./Dockerfile -t "coin-yolo" .
docker image prune --force
