# SPDX-License-Identifier: (GPL-2.0 OR BSD-2-Clause)

XDP_TARGETS := xdp_fwd_kern
XDP_TARGETS += xdp_fwd_time_kern
XDP_TARGETS += xdp_fwd_fb_kern
USER_TARGETS := xdp_fwd_user
USER_TARGETS += xdp_fwd_loader

LIB_OBJS += ../common/ext_xdp_user_utils.o

LIB_DIR = /opt/xdp-tools/lib
include $(LIB_DIR)/common.mk
