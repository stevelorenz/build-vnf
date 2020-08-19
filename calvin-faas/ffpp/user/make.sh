#!/bin/bash

make clean
make all
make examples

if [[ $1 == "-a" ]]; then
    make related_works
fi
