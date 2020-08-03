all_related_works = [
  'l2fwd_power',
  'power_hw_ctr',
]

foreach work: all_related_works
  name = work.split('/')[-1]
  sources=[]
  subdir(work)
  executable('ffpp_' + name, sources,
    include_directories: [inc, include_directories(work)],
    dependencies: [ffpp_deps],
    link_with : [ffpplib_shared])
endforeach
