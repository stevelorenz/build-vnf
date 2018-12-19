#! /bin/bash
#
# About: Check if AES-NI speedup is enabled with OpenSSL
#

if grep -q -m1 -o aes /proc/cpuinfo; then
    echo "AES-NI is available"
else
    echo "AES-NI is not available"
    exit
fi

CIPHER="aes-128-cbc"
echo "Run $CIPHER without AES-NI"
openssl speed -elapsed $CIPHER

echo "Run $CIPHER with AES-NI"
openssl speed -elapsed -evp $CIPHER
