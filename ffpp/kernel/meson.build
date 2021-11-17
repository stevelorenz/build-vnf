# Reference: https://github.com/xdp-project/xdp-tools/blob/master/lib/common.mk
xdp_build_cmd = [
  'clang',
  '-target', 'bpf',
  '-D', '__BPF_TRACING__',
  '-O2', '-g', '-Wall',
  '-Wno-compare-distinct-pointer-types',
  ]

# ISSUE: Now only kernel space XDP programs are built into Meson's build directory.
# Userspace functionalities
subdir('xdp_count')
subdir('xdp_drop')
subdir('xdp_fwd')
subdir('xdp_fwd_two_vnf')
subdir('xdp_pass')
subdir('xdp_time')
