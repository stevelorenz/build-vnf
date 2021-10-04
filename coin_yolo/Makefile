all: release

TARGET_DIR=build

mk_build_dir:
	@if [ ! -d $(TARGET_DIR) ]; then mkdir $(TARGET_DIR); fi

release: mk_build_dir
	meson build -Db_ndebug=true --buildtype=release
	cd $(TARGET_DIR) && ninja

clean:
	rm -rf $(TARGET_DIR)

rebuild:
	cd $(TARGET_DIR) && ninja
