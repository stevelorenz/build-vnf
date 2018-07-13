#
# Makefile for eBPF programs
#

CLANG  ?= clang
LLC    ?= llc
HOSTCC ?= gcc

#FLAGS := -I/home/vagrant/linux/include -O2 -Wall
FLAGS := -O2 -Wall

all:
	@echo "Makefile needs your attention"

xdp_drop:
	$(CLANG) $(FLAGS) -target bpf -c xdp_drop.c -o xdp_drop.o

# vim:ft=make
#
