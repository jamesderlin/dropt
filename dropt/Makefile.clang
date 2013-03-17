# dropt Makefile for GNU make
#
# Written by James D. Lin and assigned to the public domain.
#
# The latest version of this file can be downloaded from:
# <http://www.taenarum.com/software/dropt/>

.SILENT:
.SUFFIXES:

ifndef SRC_DIR
SRC_DIR := $(CURDIR)
endif

ifndef BUILD_ROOT
BUILD_ROOT := $(SRC_DIR)/build
endif

CC := gcc
CXX := g++
AR := ar

CFLAGS := -g -Wall
ARFLAGS := rcs

DEBUG_SUFFIX :=
NO_STRING_SUFFIX :=

ifdef DEBUG
DEBUG_SUFFIX := -debug
CFLAGS += -O0 -DDDROPT_DEBUG_STRING_BUFFERS
else
CFLAGS += -O3 -DNDEBUG
endif

ifdef DROPT_NO_STRING_BUFFERS
NO_STRING_SUFFIX := -nostring
CFLAGS += -DDROPT_NO_STRING_BUFFERS
endif

ifdef _UNICODE
UNICODE_SUFFIX := -w
CFLAGS += -D_UNICODE
endif

OUT_DIR := $(BUILD_ROOT)/lib$(DEBUG_SUFFIX)$(NO_STRING_SUFFIX)$(UNICODE_SUFFIX)
OBJ_DIR := $(BUILD_ROOT)/tmp$(DEBUG_SUFFIX)$(NO_STRING_SUFFIX)$(UNICODE_SUFFIX)

GLOBAL_DEP := $(SRC_DIR)/dropt.h $(SRC_DIR)/dropt_string.h
LIB_OBJ_FILES := $(OBJ_DIR)/dropt.o $(OBJ_DIR)/dropt_handlers.o $(OBJ_DIR)/dropt_string.o
OBJ_FILES := $(LIB_OBJ_FILES) $(OBJ_DIR)/dropt_example.o $(OBJ_DIR)/test_dropt.o

DROPT_LIB := $(OUT_DIR)/libdropt.a
DROPTXX_LIB := $(OUT_DIR)/libdroptxx.a
EXAMPLE_EXE := $(OBJ_DIR)/dropt_example
TEST_EXE := $(OBJ_DIR)/test_dropt


# Targets --------------------------------------------------------------

.PHONY: default all lib libxx
default: lib libxx
all: default example test
lib: $(DROPT_LIB)
libxx: $(DROPTXX_LIB)


.PHONY: example skip_example
ifdef DROPT_NO_STRING_BUFFERS
example: skip_example
else
ifdef _UNICODE
example: skip_example
else
example: $(EXAMPLE_EXE)
endif
endif

skip_example:
	@echo "(Skipping dropt_example because either DROPT_NO_STRING_BUFFERS or _UNICODE was specified.)"


.PHONY: check test
check: test
ifdef _UNICODE
test:
	@echo "(Skipping tests because _UNICODE was specified for gcc.)"
else
test: $(TEST_EXE)
	@echo "Running tests..."
	$(TEST_EXE) $(TEST_DROPT_ARGS)
	@echo "Tests passed."
endif


$(DROPT_LIB) $(DROPTXX_LIB): $(LIB_OBJ_FILES)
	-mkdir -p $(@D)
	$(AR) $(ARFLAGS) $@ $^

$(DROPTXX_LIB): $(OBJ_DIR)/droptxx.o

$(OBJ_DIR)/%: $(OBJ_DIR)/%.o $(DROPT_LIB)
	$(CC) $(CFLAGS) $< -L$(OUT_DIR) -ldropt -o $@

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c $(GLOBAL_DEP)
	-mkdir -p $(@D)
	$(CC) -c -o $@ $(CFLAGS) $<
	@echo "$(<F)"

$(OBJ_DIR)/droptxx.o: $(SRC_DIR)/droptxx.cpp $(GLOBAL_DEP) $(SRC_DIR)/droptxx.hpp
	-mkdir -p $(@D)
	$(CXX) -c -o $@ $(CFLAGS) $(CXXFLAGS) $<
	@echo "$(<F)"


# Directories ----------------------------------------------------------

.PHONY: clean
clean:
	-rm -rf "$(BUILD_ROOT)"
