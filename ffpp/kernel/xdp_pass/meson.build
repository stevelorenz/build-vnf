xdp_pass_kern = custom_target('xdp_pass_kern',
  output : 'xdp_pass_kern.o',
  input : 'xdp_pass_kern.c',
  command : xdp_build_cmd + ['-c', '@INPUT@', '-o', '@OUTPUT@'],
  install : false,
  build_by_default: true,
  )
