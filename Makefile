# Compiler and flags
CXX = clang++
CXXFLAGS = -msimd128 -I. -Isrc -std=c++17
WASM_FLAGS = --target=wasm32-unknown-wasi -mexec-model=reactor -fno-exceptions
LDFLAGS = -Wl,--no-entry -Wl,--export-all

# Tools
WIT_BINDGEN = wit-bindgen
SED = sed
BASE64 = base64

# Project name
NAME = hll-sketch

# Directories
BUILD_DIR = build
SRC_DIR = src

# Files
WASM_FILE = $(BUILD_DIR)/extension.wasm
TAR_FILE = $(BUILD_DIR)/$(NAME).tar
WIT_FILE = $(BUILD_DIR)/extension.wit
CPP_FILES = $(SRC_DIR)/extension_impl.cpp $(SRC_DIR)/extension.cpp
LOAD_SQL_FILE = $(BUILD_DIR)/load_extension.sql

# Phony targets
.PHONY: all clean debug release gen test

# Default target
all: $(WASM_FILE)

# Debug build
debug: CXXFLAGS += -g -O0
debug: $(WASM_FILE)

# Release build
release: CXXFLAGS += -O3
release: $(WASM_FILE)

# Build the WebAssembly module
$(WASM_FILE): gen $(CPP_FILES)
	$(CXX) $(CXXFLAGS) $(WASM_FLAGS) $(LDFLAGS) -o $@ $(CPP_FILES)
	tar cvf $(TAR_FILE) -C $(BUILD_DIR) $(NAME).sql extension.wasm extension.wit

# Generate bindings
gen:
	$(WIT_BINDGEN) c -e $(WIT_FILE) --out-dir $(SRC_DIR)/
	mv $(SRC_DIR)/extension.c $(SRC_DIR)/extension.cpp
	$(SED) 's/ret->ptr = canonical_abi_realloc(NULL, 0, 1, ret->len);/ret->ptr = reinterpret_cast<char *>(canonical_abi_realloc(NULL, 0, 1, ret->len));/g' $(SRC_DIR)/extension.cpp > $(SRC_DIR)/extension.cpp.tmp && mv $(SRC_DIR)/extension.cpp.tmp $(SRC_DIR)/extension.cpp

# Clean build artifacts
clean:
	rm -f $(TAR_FILE)
	rm -f $(WASM_FILE)
	rm -f $(SRC_DIR)/extension.cpp
	rm -f $(SRC_DIR)/extension.h

# Run tests
test: debug
	writ --expect 8 --wit $(WIT_FILE) $(WASM_FILE) $(NAME) 2 3
	@echo PASS
	writ --expect 1 --wit $(WIT_FILE) $(WASM_FILE) $(NAME) 2 0
	@echo PASS
	writ --expect 0 --wit $(WIT_FILE) $(WASM_FILE) $(NAME) 0 2
	@echo PASS