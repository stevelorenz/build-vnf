#!/bin/bash
#
# About: Common cleanups inside development VM
#

echo "Remove all containers."
docker container rm -f $(docker ps -aq)

echo "Remove all dangling docker images."
docker rmi $(docker images -f "dangling=true" -q)
