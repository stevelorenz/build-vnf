#!/bin/bash

echo "* Build FFPP release Docker image."
docker build -f ./Dockerfile -t ffpp .
docker image prune --force
