#! /bin/sh
#
# About: Build Linux kernel from source
#


KERNEL_RELEASE="v4.18-rc4"

if [[ ! -d $HOME/linux ]]; then
    echo "[INFO] Clone Linux source."
    cd $HOME || exit
    echo "Start cloning, it may take some time."
    git clone https://github.com/torvalds/linux.git
else
    echo "[INFO] Linux source already cloned."
fi

sudo apt update
sudo apt install -y libncurses5-dev gcc make git exuberant-ctags bc libssl-dev

cd "$HOME/linux/" || exit

# Duplicate current config
cp /boot/config-`uname -r`* .config
