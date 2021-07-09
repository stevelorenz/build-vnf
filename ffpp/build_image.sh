#!/bin/bash

echo "* Build FFPP development Docker image."
docker build -t ffpp .
docker image prune --force
