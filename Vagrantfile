# -*- mode: ruby -*-
# vi: set ft=ruby :

###############
#  Variables  #
###############

# Reduce this if your computer does not have enough CPU cores.
CPUS = 4
# 4GB is required by some deep learning applications, typical network functions
# do not require so much memory.
# 2GB should be enough to test most network functions.
RAM = 4096

# Use Ubuntu LTS for dev.
# Bento box is relative light weight.
BOX = "bento/ubuntu-20.04"

#######################
#  Provision Scripts  #
#######################

# Update mirrors and install some basic tools
$bootstrap= <<-SCRIPT
APT_PKGS=(
  bash-completion
  curl
  dfc
  gdb
  git
  htop
  pkg-config
  python3
  python3-pip
  tcpdump
  wget
)
DEBIAN_FRONTEND=noninteractive apt-get update
DEBIAN_FRONTEND=noninteractive apt-get upgrade -y
DEBIAN_FRONTEND=noninteractive apt-get install -y "${APT_PKGS[@]}"
SCRIPT

# For latest kernel features supporting eBPF and XDP (especially the libxdp in xdp-tools).
# 5.10 is required for multi-attach feature provided by xdp-tool.
# 5.11 is required for busy-pooling with AF_XDP (DPDK AF_XDP PMD supports busy polling for performance).
# Besides the kernel, linux-tools are also installed for tools like perf (a.k.a perf_events).
$install_kernel= <<-SCRIPT
DEBIAN_FRONTEND=noninteractive apt-get update
APT_PKGS=(
  linux-headers-5.11.0-25-generic
  linux-image-5.11.0-25-generic
  linux-tools-5.11.0-25-generic
  linux-tools-common
  linux-tools-generic
)
DEBIAN_FRONTEND=noninteractive apt-get install -y "${APT_PKGS[@]}"
SCRIPT

# Hotspot is a GUI tool to visualize perf record
$install_devtools=<<-SCRIPT
APT_PKGS=(
  hotspot
  valgrind
)
DEBIAN_FRONTEND=noninteractive apt-get update
DEBIAN_FRONTEND=noninteractive apt-get install -y "${APT_PKGS[@]}"
SCRIPT

$setup_dev_net= <<-SCRIPT
# Enable IPv4 forwarding
sysctl -w net.ipv4.ip_forward=1
SCRIPT

# Xorg packages are needed for X forwarding
$setup_x11_server_apt= <<-SCRIPT
APT_PKGS=(
  openbox
  xauth
  xorg
  xterm
)
DEBIAN_FRONTEND=noninteractive apt-get update
DEBIAN_FRONTEND=noninteractive apt-get install -y "${APT_PKGS[@]}"
SCRIPT

$post_installation= <<-SCRIPT
# Allow vagrant user to use Docker without sudo
usermod -aG docker vagrant
if [ -d /home/vagrant/.docker ]; then
  chown -R vagrant:vagrant /home/vagrant/.docker
fi

DEBIAN_FRONTEND=noninteractive apt-get autoclean -y
DEBIAN_FRONTEND=noninteractive apt-get autoremove -y
SCRIPT

####################
#  Vagrant Config  #
####################

Vagrant.configure("2") do |config|

  config.vm.define "dev" do |dev|
  # --- VM for network function (dev)elopment/test/benchmarking ---

    dev.vm.provider "virtualbox" do |vb|
      vb.name = "build-vnf-dev"
      vb.memory = RAM
      vb.cpus = CPUS
      # These configs are needed to let the VM has SSE4 SIMD instructions. They are required by DPDK.
      vb.customize ["setextradata", :id, "VBoxInternal/CPUM/SSE4.1", "1"]
      vb.customize ["setextradata", :id, "VBoxInternal/CPUM/SSE4.2", "1"]
    end

    dev.vm.box = BOX
    dev.vm.synced_folder ".", "/vagrant"

    dev.vm.hostname="build-vnf-dev"
    dev.vm.box_check_update= true

    dev.vm.provision "shell", inline: $bootstrap, privileged: true
    dev.vm.provision "shell", inline: $install_kernel, privileged: true, reboot: true  # reboot is needed to load the new kernel
    dev.vm.provision "shell", inline: $install_devtools, privileged: true
    dev.vm.provision "shell", inline: $setup_x11_server_apt, privileged: true
    # Install related projects: ComNetsEmu (network emulator). It's used for local development and tests based on network emulation.
    dev.vm.provision "shell", privileged:false ,path: "./scripts/install_comnetsemu.sh", reboot: true
    dev.vm.provision "shell", inline: $post_installation, privileged: true, reboot: true

    # Always run this when use `vagrant up`
    # - Drop the system caches to allocate hugepages immediately after boot
    dev.vm.provision "shell", privileged:true, run: "always", path: "./scripts/setup_hugepage.sh"
    dev.vm.provision "shell", privileged:false, run: "always", path: "./scripts/echo_banner.sh"

    dev.ssh.forward_agent = true
    dev.ssh.forward_x11 = true
  end

end
