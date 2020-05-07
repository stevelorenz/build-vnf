#!/bin/bash
#
# About: Setup Rust (nightly) development environment on Debian/Ubuntu.
#        Alarm! Just for fun. Use stable package from the package manager if seriously.
#

curl https://sh.rustup.rs -sSf | sh -s -- --default-toolchain nightly -y
echo 'export PATH="$PATH:~/.cargo/bin"' >>~/.bashrc
