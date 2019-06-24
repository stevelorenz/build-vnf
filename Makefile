#
# Makefile
#

all:
	@echo "Makefile needs your attention"

.PHONY: doc-html
doc-html:
	@echo "Build documentation in HTML with Sphinx"
	cd ./doc && make html


# vim:ft=make
#
