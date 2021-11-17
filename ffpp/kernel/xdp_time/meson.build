xdp_time_kern = custom_target('xdp_time_kern',
  output : 'xdp_time_kern.o',
  input : 'xdp_time_kern.c',
  command : xdp_build_cmd + ['-I ./common_kern_user.h', '-c', '@INPUT@', '-o', '@OUTPUT@'],
  install : false,
  build_by_default: true,
  )
