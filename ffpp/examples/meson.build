all_examples = [
	'2_vnf_manager',
	'c1_manager',
	'feedback_manager',
	'frequency_manager',
	'frequency_switcher',
	'power_manager',
	'traffic_monitor',
]

foreach example: all_examples
	name = example.split('/')[-1]
	sources=[]
	subdir(example)
	executable('ffpp_' + name, sources,
		include_directories: [inc, include_directories(example)],
		dependencies: ffpp_deps,
		link_with : [ffpplib_shared])
	endforeach
