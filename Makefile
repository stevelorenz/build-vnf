#
# Makefile
#

all:
	@echo "Makefile needs your attention"

.PHONY: doc-html rm-all-containers rm-dangling-images
doc-html:
	@echo "Build documentation in HTML with Sphinx"
	cd ./doc && make html

## Cleanup utilities

rm-all-containers:
	@echo "Remove all docker containers"
	docker container rm $$(docker ps -aq) -f

rm-dangling-images:
	@echo "Remove all dangling docker images"
	docker rmi $$(docker images -f "dangling=true" -q)


# vim:ft=make
#
