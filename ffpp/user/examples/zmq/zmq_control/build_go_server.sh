#!/bin/bash
#
# build_go_server.sh
#

echo "Download dependency"
go get github.com/docker/docker/client
go get github.com/pebbe/zmq4

echo "Build go version zmq server"
go build -o server.out ./server.go
