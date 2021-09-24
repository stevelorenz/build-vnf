#!/bin/bash

version=$(<./VERSION)
docker build -f ./Dockerfile -t "ffpp:$version" .
docker image prune --force
