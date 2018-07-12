#! /bin/sh
#
# setup_dev.sh
# About: Setup the development environment for eBPF programs
#

KERNEL_RELEASE="v4.18-rc4"

sudo apt-get install -y make gcc libssl-dev bc libelf-dev libcap-dev \
    clang gcc-multilib llvm libncurses5-dev git pkg-config libmnl0 bison flex \
    graphviz

if [[ ! -d $HOME/linux ]]; then
    echo "[INFO] Clone Linux source."
    cd $HOME || exit
    echo "Start cloning, it may take some time."
    git clone https://github.com/torvalds/linux.git
else
    echo "[INFO] Linux source already cloned."
fi

cd "$HOME/linux/" || exit
git checkout -b $KERNEL_RELEASE $KERNEL_RELEASE
git checkout $KERNEL_RELEASE

echo "[INFO] Verifier the setup"
cd "$HOME/linux/tools/testing/selftests/bpf//" || exit
make
sudo ./test_verifier
