ffpp_dep = dependency('libffpp')

all_related_works = [
  'l2fwd_power',
]

foreach work: all_related_works
  name = work.split('/')[-1]
  sources=[]
  subdir(work)
  executable('ffpp_' + name, sources,
    include_directories: [inc, include_directories(work)],
    dependencies: [dpdk_dep, bpf_dep, ffpp_dep],
    link_with : [ffpplib_shared])
endforeach