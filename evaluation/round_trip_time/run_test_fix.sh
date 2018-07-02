#! /bin/bash
#
# About: Run RTT measurements with a fixed chain length

SERVER_ADDR="10.0.0.4:6666"
CLIENT_ADDR="10.0.0.7"
CSV_FILE="./udp_rtt_dpdk_xor_1400B_5ms.csv"
PAYLOAD_SIZE="1400"

for (( i = 0; i < "$1"; i++ )); do
    echo "# Push out-dated segments..."
    python3 ../../delay_timer/udp_owd_timer.py -c $SERVER_ADDR --ipd 0.05 --payload_size $PAYLOAD_SIZE --log DEBUG -n 50
    sleep 60
    echo "# Restart the UDP server..."
    echo "# Run UDP client..."
    python3 ../../delay_timer/udp_rtt_timer.py -c $SERVER_ADDR -l $CLIENT_ADDR --ipd 0.05 --payload_size $PAYLOAD_SIZE --log DEBUG --csv_path $CSV_FILE -n 500
done
