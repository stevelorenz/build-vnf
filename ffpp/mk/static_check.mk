BASH_SRCS := $(shell find ./ -name '*.sh')
PY_SRCS := $(shell find ./ -name '*.py')
ALL_SCRIPT_SRCS := $(BASH_SRCS) $(PY_SRCS)
C_SRCS := $(shell find ./ -name "*.c")
C_HEADERS := $(shell find ./ -name "*.h")
ALL_C_FILES := $(C_SRCS) $(C_HEADERS)
CPP_SRCS := $(shell find ./ -name "*.cpp")
CPP_HEADERS := $(shell find ./ -name "*.hpp")
ALL_CPP_FILES := $(CPP_SRCS) $(CPP_HEADERS)

.PHONY: codecheck-scripts
codecheck-scripts: $(ALL_SCRIPT_SRCS)
	@echo "* Run flake8 for python sources:"
	flake8 --ignore=E501,E266,E203,E231,W503,F401,F841 --max-complexity 10 $(PY_SRCS) || true
	@echo "* Run shellcheck for BASH sources..."
	shellcheck $(BASH_SRCS)

codecheck-cxx: $(ALL_C_FILES) $(ALL_CPP_FILES)
	@echo "* Run cppcheck: "
	cd $(TARGET_DIR) && ninja cppcheck

.PHONY: flawfinder-cxx
flawfinder-cxx: $(ALL_C_FILES) $(ALL_CPP_FILES)
	@echo "* Check torrential flaws and vulnerabilities with static checker:"
	flawfinder --minlevel 2 $(ALL_C_FILES) $(ALL_CPP_FILES)

.PHONY: format-scripts
format-scripts: $(ALL_SCRIPT_SRCS)
	@echo "* Format all python sources with black:"
	black $(PY_SRCS)  || true
	@echo "* Format all bash sources with shfmt:"
	shfmt -i 4 -w $(BASH_SRCS)

.PHONY: format-cxx
format-cxx: $(ALL_C_FILES) $(ALL_CPP_FILES)
	@echo "* Format all c and cpp sources with clang-format"
	clang-format --style=file -i $(ALL_C_FILES) $(ALL_CPP_FILES)
