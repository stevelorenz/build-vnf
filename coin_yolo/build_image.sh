#!/bin/bash

set -Eeuo pipefail
DOCKER_BUILDKIT=1 docker build -f ./Dockerfile -t "coin_yolo:latest" .
docker image prune --force
