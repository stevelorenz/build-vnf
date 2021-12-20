#!/bin/bash

if [[ ! -d ./build ]]; then
	meson build
fi

cd ./build || exit
ninja install
ninja uninstall
