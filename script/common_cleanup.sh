#! /bin/bash
#
# About: Run common cleanups for the project

echo "# Remove old vagrant boxes"
cd ../
vagrant box prune
