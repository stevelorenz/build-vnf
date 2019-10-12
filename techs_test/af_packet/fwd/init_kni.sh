#!/bin/bash
#
# About: Init KNI interfaces and related stuffs
#

IN_IFCE="vEth0"
OUT_IFCE="vEth1"
IN_IP="10.0.0.11/28"
OUT_IP="10.0.0.12/28"

SRC_IP="10.0.0.13"
SRC_MAC="08:00:27:1e:2d:b3"
DST_IP="10.0.0.14"
DST_MAC="08:00:27:e1:f1:7d"

while true; do
    if sudo ip link set "$IN_IFCE" up; then
        break
    fi
done

while true; do
    if sudo ip link set "$OUT_IFCE" up; then
        break
    fi
done

sudo ip addr add "$IN_IP" dev "$IN_IFCE"
sudo ip addr add "$OUT_IP" dev "$OUT_IFCE"
sudo arp -i "$IN_IFCE" -s "$SRC_IP" "$SRC_MAC"
sudo arp -i "$OUT_IFCE" -s "$SRC_IP" "$SRC_MAC"
sudo arp -i "$IN_IFCE" -s "$DST_IP" "$DST_MAC"
sudo arp -i "$OUT_IFCE" -s "$DST_IP" "$DST_MAC"

echo "Init KNI intefaces finished."
