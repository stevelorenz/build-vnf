#!/bin/bash

set -Eeuo pipefail
DOCKER_BUILDKIT=1 docker build -f ./Dockerfile -t "coin_dl:0.1.0" .
docker image prune --force
