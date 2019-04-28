# Ref: https://bitbucket.org/comnets/mp-congestion/src/96eb40fa2b71b796cf953288e1ad74b230b81798/paper/Makefile?at=master&fileviewer=file-view-default

DOCUMENT=problem_formulation
LOG=$(DOCUMENT).log
ROOT=$(shell pwd)

TEX_SRCS=$(DOCUMENT).tex
BIB_SRCS=bibtex.bib

BUILD_DIR=.build
BUILD_STAGE1=$(BUILD_DIR)/$(DOCUMENT).s1.pdf

.PHONY: show clean

all: $(DOCUMENT).pdf

$(BUILD_STAGE1): $(TEX_SRCS) $(IMGS)
	mkdir -p $(BUILD_DIR)
	pdflatex -output-directory=$(BUILD_DIR) $(DOCUMENT).tex 2>&1 | tee $(LOG)
	mv $(BUILD_DIR)/$(DOCUMENT).pdf $@

$(DOCUMENT).pdf: $(TEX_SRCS) $(IMGS)
	mkdir -p $(BUILD_DIR)
	pdflatex -output-directory=$(BUILD_DIR) $(DOCUMENT).tex 2>&1 | tee $(LOG)
	cd $(BUILD_DIR) && BIBINPUTS=$(ROOT) biber $(DOCUMENT)
	pdflatex -output-directory=$(BUILD_DIR) $(DOCUMENT).tex 2>&1 | tee $(LOG)
	pdflatex -output-directory=$(BUILD_DIR) $(DOCUMENT).tex 2>&1 | tee $(LOG)
	mv $(BUILD_DIR)/$(DOCUMENT).pdf $@

show: $(DOCUMENT).pdf
	xdg-open $(DOCUMENT).pdf

clean:
	-rm -r $(BUILD_DIR)
	-rm -fv *.aux *.log *.bbl *.blg *.toc *.out *.lot *.lof *.gz
	-rm $(DOCUMENT).pdf
