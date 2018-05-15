#!/bin/bash
#
# About: Allocate 2048kB(2MB) hugepages

echo $1 | sudo tee /sys/kernel/mm/hugepages/hugepages-2048kB/nr_hugepages
cat /sys/kernel/mm/hugepages/hugepages-2048kB/nr_hugepages
