debug: mk_build_dir
	@echo "* Build the libraries with debug mode (sanitizers are enabled)"
	CC=$(CC) CXX=$(CXX) meson -Db_sanitize=address $(TARGET_DIR) --buildtype=debug
	cd $(TARGET_DIR) && ninja
	cd ./kern && make DEBUG=1

tests-debug: debug
	@echo "* Build tests with debug mode (sanitizers are enabled)"
	@echo "* Warning: the build can take long time due to sanitizers"
	meson configure -Dtests=true -Db_coverage=true -Db_sanitize=address $(TARGET_DIR) --buildtype=debug
	cd $(TARGET_DIR) && ninja

run-benchmark:
	@echo "* Run benchmark using google/benchmark"
	cd $(TARGET_DIR)/benchmark && ./gbenchmark

# Disable detecting memory leaks: ASAN_OPTIONS=detect_leaks=0 meson test --print-errorlogs
run-tests:
	@echo "* Run all tests"
	cd $(TARGET_DIR) && meson test --print-errorlogs

run-tests-v:
	@echo "* Run all tests with verbose mode"
	cd $(TARGET_DIR) && meson test --print-errorlogs --verbose

run-tests-dev:
	@echo "* Only run tests of suite dev with verbose mode"
	cd $(TARGET_DIR) && meson test --suite dev  --print-errorlogs --verbose

run-coverage: tests
	@echo "* Generate coverage report (HTML)"
	cd $(TARGET_DIR) && ninja test && ninja coverage-html

.PHONY: rm-all-containers
rm-all-containers:
	@echo "Remove all docker containers"
	docker container rm $$(docker ps -aq) -f

.PHONY: rm-dangling-images
rm-dangling-images:
	@echo "Remove all dangling docker images"
	docker rmi $$(docker images -f "dangling=true" -q)
