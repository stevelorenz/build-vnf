# MoonGen Guide

Tip when configuring devices
local dev = device.config{dropEnable = false}
--- dropEnable optional (default = true) Drop packets directly if no RX descriptors are available
but sometimes NIC doesn't support this.

82540EM doesn't support Hardware-timestamping

function mod.sleepMillis(t) in libmoon.lua
--- Delay by t milliseconds. Note that this does not sleep the actual thread;
--- the time is spent in a busy wait loop by DPDK.
