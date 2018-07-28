#! /bin/bash
#
# setup_moongen.sh
# About: Install and setup MoonGen packet generator
#
#


if [[ $1 = '-i' ]]; then
    echo "# Install MoonGen from source code"
    sudo apt-get install -y build-essential cmake linux-headers-`uname -r` pciutils libnuma-dev
    git clone https://github.com/emmericp/MoonGen $HOME/MoonGen
    cd $HOME/MoonGen || exit
    ./build.sh

elif [[ $1 = '-s' ]]; then
    echo "# Setup MoonGen"
    cd $HOME/MoonGen || exit
    sudo ./bind-interfaces.sh
    sudo ./setup-hugetlbfs.sh
else
    echo "# Unknown option."
    echo " -i: Install MoonGen"
    echo " -s: Setup MoonGen (e.g. After reboot)"
fi
