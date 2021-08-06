all: release

release:
	cd ./common/ && make
	cd ./xdp_drop/ && make
	cd ./xdp_pass/ && make
	cd ./xdp_count/ && make
	cd ./xdp_fwd/ && make
	cd ./xdp_time && make

debug:
	cd ./common/ && make DEBUG=1
	cd ./xdp_drop/ && make DEBUG=1
	cd ./xdp_pass/ && make DEBUG=1
	cd ./xdp_count/ && make DEBUG=1
	cd ./xdp_fwd/ && make DEBUG=1
	cd ./xdp_time && make DEBUG=1

# Some special object files (e.g. the dispatch.o from libxdp) is not removed via `make clean`
OBJ_FILES := $(shell find ./ -name '*.o')

clean:
	cd ./common/ && make clean
	cd ./xdp_drop/ && make clean
	cd ./xdp_pass/ && make clean
	cd ./xdp_count/ && make clean
	cd ./xdp_fwd/ && make clean
	cd ./xdp_time && make clean
	@rm $(OBJ_FILES)

.PHONY: release debug clean