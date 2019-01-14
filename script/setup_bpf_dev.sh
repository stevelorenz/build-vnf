#! /bin/bash
#
# About: Set up eBPF development environment
#

KERNEL_VERSION=$1
echo "Kernel version: $1"

sudo apt update
sudo apt install -y clang llvm gcc make
exit 0

cd || exit
echo "Download kernel source code"
wget https://cdn.kernel.org/pub/linux/kernel/v4.x/linux-$1.tar.xz
tar -xf ./linux-$1.tar.xz


echo "Build BPF library"
sudo apt-get install -y libelf-dev
cd $HOME/linux-4.19/tools/lib/bpf || exit
make -j 2
sudo mkdir -p /opt/libbpf
sudo chown -R vagrant:vagrant /opt/libbpf
cp libbpf.so libbpf.a /opt/libbpf
mkdir -p /opt/libbpf/include/bpf
cp bpf.h libbpf.h /opt/libbpf/include/bpf

# Copy some headers to avoid compilation errors, not clean approach but works...
sudo cp $HOME/linux-4.19/include/uapi/linux/if_xdp.h /usr/include/linux/if_xdp.h
sudo cp $HOME/linux-4.19/include/uapi/linux/if_link.h /usr/include/linux/if_link.h
sudo cp $HOME/linux-4.19/include/uapi/linux/bpf.h  /usr/include/linux/bpf.h

echo "Build AF_XDP sample program"
gcc xdpsock_user.c -o xdpsock_user -lpthread -L/opt/libbpf -lbpf -lelf  -I/opt/libbpf/include -I$HOME/linux-4.19/tools/testing/selftests/bpf
clang -c  xdpsock_kern.c  -I$HOME/linux-4.19/include -I$HOME/linux-4.19/tools/testing/selftests/bpf -D__KERNEL__ -D__BPF_TRACING__ -Wno-unused-value -Wno-pointer-sign  -Wno-compare-distinct-pointer-types -Wno-gnu-variable-sized-type-not-at-end -Wno-address-of-packed-member -Wno-tautological-compare   -Wno-unknown-warning-option  -O2 -emit-llvm -c -o -| llc -march=bpf -filetype=obj  -o xdpsock_user_kern.o
