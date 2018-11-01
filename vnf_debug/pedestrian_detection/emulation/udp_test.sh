#! /bin/bash
#
# About: UDP test with netcat
#

NUM=5
SRV_IP="10.0.0.100"
SRV_PORT=6666

if [[ $1 == "-s" ]]; then
    echo "Run UDP server on $SRV_IP:$SRV_PORT."
    for (( i = 0; i < NUM; i++ )); do
        netcat -l -p $SRV_PORT -u -w0 $SRV_IP
    done
elif [[ $1 == "-c" ]]; then
    echo "Run UDP client. Send to $SRV_IP:$SRV_PORT"
    for (( i = 0; i < NUM; i++ )); do
        echo -e "Pikachu!" | nc -u -w0 $SRV_IP $SRV_PORT
        sleep 1
    done
fi
