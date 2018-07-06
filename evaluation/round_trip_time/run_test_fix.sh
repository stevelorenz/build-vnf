#! /bin/bash
#
# About: Run RTT measurements with a fixed chain length
#        Put this file to the ../delay_timer/

SERVER_ADDR="10.0.0.4:6666"
CLIENT_ADDR="10.0.0.7"
CSV_FILE="./udp_rtt_click_xor_1400B_5ms.csv"
PAYLOAD_SIZE="1400"

if [[ "$1" = "push" ]]
then
    for ((i = 0; i < 5; i++)); do
        echo "Try to push the channel..."
        python3 ./udp_owd_timer.py -c $SERVER_ADDR --bind $CLIENT_ADDR:9999 --ipd 0.05 --payload_size $PAYLOAD_SIZE --log INFO -n 50
    done
fi

for (( i = 0; i < "$1"; i++ )); do
    echo "# Run UDP client..."
    python3 ./udp_rtt_timer.py -c $SERVER_ADDR --bind $CLIENT_ADDR:9999 -l $CLIENT_ADDR --ipd 0.05 --payload_size $PAYLOAD_SIZE --log DEBUG --csv_path $CSV_FILE -n 500 --push
    sleep 5
done
