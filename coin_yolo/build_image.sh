#!/bin/bash

# It takes about 660 seconds to finish without caching
DOCKER_BUILDKIT=1 docker build -f ./Dockerfile -t "coin_yolo:latest" .
docker image prune --force
