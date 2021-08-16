#!/bin/bash

echo "* Build FFPP development Docker image."
docker build -f ./Dockerfile.dev -t ffpp-dev .
docker image prune --force
