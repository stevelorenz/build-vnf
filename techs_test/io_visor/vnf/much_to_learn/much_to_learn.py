#! /usr/bin/env python2
# -*- coding: utf-8 -*-
# vim:fenc=utf-8

"""
About: Try to "play" with BCC...
Email: xianglinks@gmail.com
"""

from __future__ import print_function

from time import sleep

from bcc import BPF


def test_trace_block_io():
    """Test BCC block IO tracing"""
    b = BPF(
        text="""
    #include <uapi/linux/ptrace.h>
    #include <linux/blkdev.h>
    BPF_HISTOGRAM(dist);
    int kprobe__blk_account_io_completion(struct pt_regs *ctx, struct request *req)
    {
            dist.increment(bpf_log2l(req->__data_len / 1024));
            return 0;
    }
    """
    )

    # header
    print("Tracing... Hit Ctrl-C to end.")

    # trace until Ctrl-C
    try:
        sleep(99999999)
    except KeyboardInterrupt:
        print()

    # output
    b["dist"].print_log2_hist("kbytes")


if __name__ == "__main__":
    test_trace_block_io()
