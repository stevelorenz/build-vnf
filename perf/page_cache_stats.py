#! /usr/bin/env python2
# -*- coding: utf-8 -*-
# vim:fenc=utf-8

"""
About: Monitor CPU cache stats using eBPF
"""

import re
import signal
import sys
from time import sleep

from bcc import BPF

bpf_text = """
#include <uapi/linux/ptrace.h>

struct key_t {
    u64 ip;
};

BPF_HASH(counts, struct key_t);

int do_count(struct pt_regs *ctx) {
    struct key_t key = {};
    u64 ip;

    key.ip = PT_REGS_IP(ctx);
    counts.increment(key);
    return 0;
}
"""

# Load BPF program
b = BPF(text=bpf_text)

# Attach BPF to kernel probes (kprobes)
b.attach_kprobe(event="add_to_page_cache_lru", fn_name="do_count")
