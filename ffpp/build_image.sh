#!/bin/bash

set -Eeuo pipefail

version=$(<./VERSION)
DOCKER_BUILDKIT=1 docker build -f ./Dockerfile -t "ffpp:$version" .
docker image prune --force
