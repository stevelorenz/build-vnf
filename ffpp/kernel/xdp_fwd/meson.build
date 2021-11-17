xdp_fwd_kern = custom_target('xdp_fwd_kern',
  output : 'xdp_fwd_kern.o',
  input : 'xdp_fwd_kern.c',
  command : xdp_build_cmd + ['-I ./common_kern_user.h', '-c', '@INPUT@', '-o', '@OUTPUT@'],
  install : false,
  build_by_default: true,
  )
