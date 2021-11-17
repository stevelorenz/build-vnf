xdp_drop_kern = custom_target('xdp_drop_kern',
  output : 'xdp_drop_kern.o',
  input : 'xdp_drop_kern.c',
  command : xdp_build_cmd + ['-c', '@INPUT@', '-o', '@OUTPUT@'],
  install : false,
  build_by_default: true,
  )
