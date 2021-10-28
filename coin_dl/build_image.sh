#!/bin/bash

set -Eeuo pipefail
DOCKER_BUILDKIT=1 docker build -f ./Dockerfile -t "coin_dl:latest" .
docker image prune --force
