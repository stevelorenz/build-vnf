#! /bin/sh
#
# setup_dev.sh
# About: Setup the development environment for eBPF programs
# Distro: Ubuntu 16.04
# Ref: Cilium BPF doc
#

KERNEL_RELEASE="v4.18-rc4"

sudo apt install -y make gcc libssl-dev bc libelf-dev libcap-dev \
    gcc-multilib libncurses5-dev git pkg-config libmnl0 bison flex \
    graphviz binutils-dev

# Remove old clang and llvm
#sudo apt purge -y clang-* llvm-*
#sudo apt auto-remove

# For kernel releases 4.16+ the BPF selftest has a dependency on LLVM 6.0+
echo "# LLVM tool chain 6.0" | sudo tee -a /etc/apt/sources.list > /dev/null
echo "deb http://apt.llvm.org/xenial/ llvm-toolchain-xenial-6.0 main" | sudo tee -a /etc/apt/sources.list > /dev/null
echo "deb-src http://apt.llvm.org/xenial/ llvm-toolchain-xenial-6.0 main" | sudo tee -a /etc/apt/sources.list > /dev/null
wget -O - https://apt.llvm.org/llvm-snapshot.gpg.key|sudo apt-key add -
sudo apt update
sudo apt install -y clang-6.0 lldb-6.0 lld-6.0

sudo ln -s /usr/bin/clang-6.0 /usr/bin/clang
sudo ln -s /usr/bin/llc-6.0 /usr/bin/llc

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

# Compile bpftool
# bpftool is an essential tool around debugging and introspection of BPF programs and maps
cd "$HOME/linux/tools/bpf/bpftool/" || exit
make
sudo make install
