# TODO:  <26-09-21, Zuo> Refactor kern sources with a better structure
# Avoid duplicated code for similar functionalities

all: release

.PHONY: release
release:
	cd ./common/ && make
	cd ./xdp_drop/ && make
	cd ./xdp_pass/ && make
	cd ./xdp_count/ && make
	cd ./xdp_fwd/ && make
	cd ./xdp_time && make

.PHONY: debug
debug:
	cd ./common/ && make DEBUG=1
	cd ./xdp_drop/ && make DEBUG=1
	cd ./xdp_pass/ && make DEBUG=1
	cd ./xdp_count/ && make DEBUG=1
	cd ./xdp_fwd/ && make DEBUG=1
	cd ./xdp_time && make DEBUG=1

# Some special object files (e.g. the dispatch.o from libxdp) is not removed via `make clean`
DISPATCHER_OBJ_FILES := $(shell find ./ -name 'xdp-dispatcher.o')

.PHONY: clean
clean:
	cd ./common/ && make clean
	cd ./xdp_drop/ && make clean
	cd ./xdp_pass/ && make clean
	cd ./xdp_count/ && make clean
	cd ./xdp_fwd/ && make clean
	cd ./xdp_time && make clean
	@rm $(DISPATCHER_OBJ_FILES)
