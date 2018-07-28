#!/bin/bash

SOCKET_MEM=100
LCORES="0"

sudo ./build/test -l $LCORES -m $SOCKET_MEM
