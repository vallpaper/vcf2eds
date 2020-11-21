CXX = g++
CXXFLAGS = -Wall -pedantic -Wextra -std=c++14 -I$(EXTERNAL_LIBS_DIR)/include -MMD -MP
CXXFLAGS_DEBUG = -g -O0
CXXFLAGS_RELEASE = -O3
LIBS_INCLUDE = -L$(EXTERNAL_LIBS_DIR)/lib
LIBS = -lvcflib -lhts -lz -lm -llzma -lbz2 -lcurl

BUILD_DIR = build

OUTPUT_DIR = bin
BIN = vcf2eds
SRC = src

EXTERNAL_DIR = external
EXTERNAL_LIBS_DIR = $(EXTERNAL_DIR)/libs

VCFLIB_STATIC_LIB = $(EXTERNAL_LIBS_DIR)/lib/libvcflib.a
VCFLIB_DIR = external/vcflib
VCFLIB_BUILD_LIB = libvcflib.a
VCFLIB_URL = git@github.com:vcflib/vcflib.git

cfiles = $(shell find $(SRC) -name "*.cpp")
cfiles_dirs_tmp = $(shell find $(SRC) -name '*.cpp' -exec dirname {} \; | uniq)
ofiles_dirs = $(patsubst %, $(BUILD_DIR)/%, $(filter-out $(SRC), $(cfiles_dirs_tmp:$(SRC)/%=%)))
ofiles = $(patsubst %.cpp, $(BUILD_DIR)/%.o, $(cfiles:$(SRC)/%=%))
dfiles = $(ofiles:.o=.d)

.PHONY: all
all: vcf2eds

run:
	$(OUTPUT_DIR)/$(BIN) -r ./data/input/Homo_sapiens.GRCh38.dna.chromosome.22.fa.gz -v ./data/input/ALL.chr22.phase3_shapeit2_mvncall_integrated_v5a.20130502.genotypes.vcf.gz -o ./data/output/output.eds

stats:
	$(OUTPUT_DIR)/$(BIN) -r ./data/input/Homo_sapiens.GRCh38.dna.chromosome.22.fa.gz -v ./data/input/ALL.chr22.phase3_shapeit2_mvncall_integrated_v5a.20130502.genotypes.vcf.gz -o ./data/output/output.eds -t

.PHONY: vcf2eds
vcf2eds: external_libs $(BUILD_DIR) $(OUTPUT_DIR) $(OUTPUT_DIR)/$(BIN)

.PHONY: external_libs
external_libs: $(EXTERNAL_LIBS_DIR)/lib $(VCFLIB_STATIC_LIB)

$(OUTPUT_DIR)/$(BIN): $(ofiles)
	$(CXX) $(LIBS_INCLUDE) -o $@ $^ $(LIBS)

$(OUTPUT_DIR):
	mkdir -p $(OUTPUT_DIR)

$(BUILD_DIR)/%.o: $(SRC)/%.cpp
	$(CXX) $(CXXFLAGS) $(CXXFLAGS_DEBUG) -c $< -o $@

$(EXTERNAL_DIR):
	mkdir -p $(EXTERNAL_DIR)

$(EXTERNAL_LIBS_DIR): $(EXTERNAL_DIR)
	mkdir -p $(EXTERNAL_LIBS_DIR)
	mkdir -p $(EXTERNAL_LIBS_DIR)/lib
	mkdir -p $(EXTERNAL_LIBS_DIR)/include

$(EXTERNAL_LIBS_DIR)/lib: $(EXTERNAL_LIBS_DIR)

$(VCFLIB_STATIC_LIB): $(VCFLIB_DIR)/lib/$(VCFLIB_BUILD_LIB) $(VCFLIB_DIR)/tabixpp/htslib/libhts.a
	cp $(VCFLIB_DIR)/lib/$(VCFLIB_BUILD_LIB) $(EXTERNAL_LIBS_DIR)/lib
	cp $(VCFLIB_DIR)/tabixpp/htslib/libhts.a $(EXTERNAL_LIBS_DIR)/lib
	mkdir -p $(EXTERNAL_LIBS_DIR)/include/vcflib
	mkdir -p $(EXTERNAL_LIBS_DIR)/include/htslib
	cp $(VCFLIB_DIR)/include/*.h $(VCFLIB_DIR)/include/*.hpp $(EXTERNAL_LIBS_DIR)/include/vcflib
	cp $(VCFLIB_DIR)/tabixpp/htslib/htslib/*.h $(EXTERNAL_LIBS_DIR)/include/htslib

$(VCFLIB_DIR)/Makefile:
	git clone --recursive $(VCFLIB_URL) $(VCFLIB_DIR)

$(VCFLIB_DIR)/lib/$(VCFLIB_BUILD_LIB): $(VCFLIB_DIR)/Makefile
	$(MAKE) -C $(VCFLIB_DIR) all

$(BUILD_DIR): $(ofiles_dirs)
	mkdir -p $(BUILD_DIR)

$(ofiles_dirs): %:
	mkdir -p $@

.PHONY: clean
clean:
	rm -rf $(EXTERNAL_LIBS_DIR)
	rm -rf $(OUTPUT_DIR)
	rm -rf $(BUILD_DIR)

.PHONY: purge
purge:
	rm -rf $(EXTERNAL_DIR)
	rm -rf $(OUTPUT_DIR)
	rm -rf $(BUILD_DIR)

-include $(dfiles)