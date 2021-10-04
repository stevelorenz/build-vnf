# -*- mode: ruby -*-
# vi: set ft=ruby :

###############
#  Variables  #
###############

CPUS = 4
RAM = 4096

# Use Ubuntu LTS for dev. bento box is relative light weight.
BOX = "bento/ubuntu-20.04"
# Box for using libvirt as the provider, bento boxes do not support libvirt.
BOX_LIBVIRT = "generic/ubuntu2004"

#######################
#  Provision Scripts  #
#######################
# These scripts ONLY work for this VM managed by the Vagrant.

$bootstrap= <<-SCRIPT
# Install dependencies
DEBIAN_FRONTEND=noninteractive apt-get update
DEBIAN_FRONTEND=noninteractive apt-get upgrade -y
DEBIAN_FRONTEND=noninteractive apt-get install -y git pkg-config gdb bash-completion htop dfc tcpdump wget curl
SCRIPT

# For latest kernel features supporting eBPF and XDP (xdp-tools).
# 5.10 is required for multi-attach feature provided by xdp-tool.
# 5.11 is required for busy-pooling with AF_XDP (DPDK AF_XDP PMD supports busy polling for performance).
$install_kernel= <<-SCRIPT
DEBIAN_FRONTEND=noninteractive apt-get update
DEBIAN_FRONTEND=noninteractive apt-get install -y linux-image-5.11.0-25-generic linux-headers-5.11.0-25-generic linux-tools-generic linux-tools-5.11.0-25-generic
SCRIPT

# Firstly install/update kernel before install other kernel-related packages.
$install_devtools=<<-SCRIPT
DEBIAN_FRONTEND=noninteractive apt-get update
DEBIAN_FRONTEND=noninteractive apt-get install -y linux-tools-common linux-tools-generic
SCRIPT

$setup_dev_net= <<-SCRIPT
# Enable IP forwarding
sysctl -w net.ipv4.ip_forward=1
SCRIPT

$setup_x11_server_apt= <<-SCRIPT
DEBIAN_FRONTEND=noninteractive apt-get update
DEBIAN_FRONTEND=noninteractive apt-get install -y xorg openbox xterm xauth
SCRIPT

$post_installation= <<-SCRIPT
# Allow vagrant user to use Docker without sudo
usermod -aG docker vagrant
if [ -d /home/vagrant/.docker ]; then
  chown -R vagrant:vagrant /home/vagrant/.docker
fi
SCRIPT

####################
#  Vagrant Config  #
####################

require 'optparse'

# Parse the provider argument
def get_provider
  ret = nil
  opt_parser = OptionParser.new do |opts|
    opts.on("--provider provider") do |provider|
      ret = provider
    end
  end
  opt_parser.parse!(ARGV)
  ret
end
provider = get_provider || "virtualbox"

Vagrant.configure("2") do |config|

  config.vm.define "dev" do |dev|
  # --- VM for Network Function dev/test/benchmarking ---

    dev.vm.provider "virtualbox" do |vb|
      vb.name = "build-vnf-dev"
      vb.memory = RAM
      vb.cpus = CPUS
      vb.customize ["setextradata", :id, "VBoxInternal/CPUM/SSE4.1", "1"]
      vb.customize ["setextradata", :id, "VBoxInternal/CPUM/SSE4.2", "1"]
    end

    dev.vm.provider "libvirt" do |libvirt|
      libvirt.driver = "kvm"
      libvirt.cpus = CPUS
      libvirt.memory = RAM
    end

    if provider == "virtualbox"
      dev.vm.box = BOX
      dev.vm.synced_folder ".", "/vagrant"
    elsif provider == "libvirt"
      dev.vm.box = BOX_LIBVIRT
      # This option does not invoke vagrant rsync automatically.
      # Run `vagrant rsync-auto dev` after the VM is booted.
      dev.vm.synced_folder ".", "/vagrant", disabled: true
      dev.vm.synced_folder ".", "/vagrant", type: 'nfs', disabled: true
      dev.vm.synced_folder ".", "/vagrant", type:'rsync'
    end

    dev.vm.hostname="build-vnf-dev"
    dev.vm.box_check_update= true

    dev.vm.provision "shell", inline: $bootstrap, privileged: true
    dev.vm.provision "shell", inline: $install_kernel, privileged: true, reboot: true
    dev.vm.provision "shell", inline: $install_devtools, privileged: true
    dev.vm.provision "shell", inline: $setup_x11_server_apt, privileged: true

    # Install related projects: ComNetsEmu
    dev.vm.provision "shell", privileged:false ,path: "./scripts/install_comnetsemu.sh", reboot: true

    dev.vm.provision "shell", inline: $post_installation, privileged: true

    # Always run this when use `vagrant up`
    # - Drop the system caches to allocate hugepages
    dev.vm.provision "shell", privileged:true, run: "always", path: "./scripts/setup_hugepage.sh"

    dev.ssh.forward_agent = true
    dev.ssh.forward_x11 = true
  end

end
