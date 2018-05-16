#!/bin/bash
# About: Provision script for test VMs

# Install dependencies
sudo apt-get update
sudo apt-get install -y bash-completion git htop

# Add termite infos
wget https://raw.githubusercontent.com/thestinger/termite/master/termite.terminfo -O /home/vagrant/termite.terminfo
tic -x /home/vagrant/termite.terminfo

# Get zuo's dotfiles
git clone https://github.com/stevelorenz/dotfiles.git /home/vagrant/dotfiles
