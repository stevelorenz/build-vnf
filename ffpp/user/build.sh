#!/bin/bash

usage="$(basename "$0") [-h] [-a, --all]

where:
    -h         show this help text
    -a, --all  build all sources including tests and related_works"

if [[ "$1" == "-h" || $1 == "--help" ]]; then
    echo "$usage"
    exit 0
fi

make clean
make all
make examples

if [[ $1 == "-a" || $1 == "-all" ]]; then
    make tests
    make related_works
fi
