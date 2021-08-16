#!/bin/bash

usage="Build the project with release mode.
By default, only core libraries are built.

$(basename "$0") [-h] [-a, --all]

where:
    -h, --help   show this help text
    -a, --all    build all sources including tests, examples and related works"

if [[ "$1" == "-h" || $1 == "--help" ]]; then
    echo "$usage"
    exit 0
fi

make clean
make all

if [[ $1 == "-a" || $1 == "-all" ]]; then
    make tests
    make examples
    make related_works
fi
