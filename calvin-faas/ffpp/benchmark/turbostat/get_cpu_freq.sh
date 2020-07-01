#! /bin/bash
# 
# About: Print CPU frequency at given intervals
#

INTERVAL="10"

while :; do
    case $1 in
    -i)
        if [ "$2" ]; then
            INTERVAL="2"
            shift
        fi
        ;;
    *)
        echo "Invalid option. Use with -i [interval]."
        break
        ;;
    esac
    shift
done

while true; do
    freqs=$(grep MHz /proc/cpuinfo)
    for i in "${freqs[@]}"; do
        echo "$i"
    done
    printf "\n"
    sleep $INTERVAL
done
