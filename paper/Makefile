# About: Makefile to build Latex files into pdf with bibtex references
# Tool : pdflatex, bibtex

# Name of the main text file
DOCUMENT=main
LOG=$(DOCUMENT).log
# Root work directory
ROOT=$(shell pwd)

TEX_SRCS=$(DOCUMENT).tex $(shell find sections -name "*.tex")
BIB_SRCS=bibtex.bib

IMGS=./figures/

BUILD_DIR=.build
BUILD_STAGE1=$(BUILD_DIR)/$(DOCUMENT).s1.pdf
BUILD_STAGE2=$(BUILD_DIR)/$(DOCUMENT).bbl
BUILD_STAGE3=$(BUILD_DIR)/$(DOCUMENT).s3.pdf

.PHONY: show clean

all: $(DOCUMENT).pdf

$(BUILD_STAGE1): $(TEX_SRCS) $(IMGS)
	mkdir -p $(BUILD_DIR)
	pdflatex -output-directory=$(BUILD_DIR) $(DOCUMENT).tex 2>&1 | tee $(LOG)
	mv $(BUILD_DIR)/$(DOCUMENT).pdf $@

$(BUILD_STAGE2): $(BUILD_STAGE1) $(BIB_SRCS)
	cd $(BUILD_DIR) && BIBINPUTS=$(ROOT) bibtex $(DOCUMENT)

$(BUILD_STAGE3): $(BUILD_STAGE2)
	pdflatex -output-directory=$(BUILD_DIR) $(DOCUMENT).tex 2>&1 | tee $(LOG)
	pdflatex -output-directory=$(BUILD_DIR) $(DOCUMENT).tex 2>&1 | tee $(LOG)
	mv $(BUILD_DIR)/$(DOCUMENT).pdf $@

$(DOCUMENT).pdf: $(BUILD_STAGE3)
	cp $< $@

show: $(DOCUMENT).pdf
	xdg-open $(DOCUMENT).pdf

clean:
	-rm -r $(BUILD_DIR)

draft: draft.tex
	mkdir -p $(BUILD_DIR)
	pdflatex -output-directory=$(BUILD_DIR) draft.tex 2>&1 | tee draft.log
	mv $(BUILD_DIR)/draft.pdf ./
	xdg-open draft.pdf
