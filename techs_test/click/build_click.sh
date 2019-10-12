#! /bin/bash
#
# About: Build Click with customized elements in /elements/local

COMPILE_ELEMENT_PATH="$HOME/click/elements/local/"

if [[ -d "./elements" ]]; then
    echo "Copy custom elements to $COMPILE_ELEMENT_PATH."
    cp ./elements/* "$COMPILE_ELEMENT_PATH"
fi

cd "$HOME/click" || exit
./configure --prefix=/usr/local --enable-local
sudo make elemlist
sudo make install
