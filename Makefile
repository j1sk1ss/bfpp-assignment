CC ?= cc
CFLAGS ?= -O3 -std=c99 -Wall -Wextra -Wpedantic -Wconversion -Wshadow
CPPFLAGS ?=
INCLUDE_DIRS := include/bf src src/core src/compiler src/io src/tape src/program src/vm tests/support tests/unit
CPPFLAGS += $(addprefix -I,$(INCLUDE_DIRS))
PYTHON ?= python3
BUILD := build-make
.DEFAULT_GOAL := all

LIB_SRC := \
	src/core/common.c src/core/object.c \
	src/io/io_base.c src/io/io_stdio.c src/io/io_buffer.c \
	src/tape/tape_base.c src/tape/tape_fixed.c src/tape/tape_grow.c \
	src/program/program.c \
	src/compiler/compiler_base.c src/compiler/compiler_classic.c src/compiler/compiler_bfpp.c src/compiler/compiler_core.c \
	src/vm/vm_switch.c
LIB_OBJ := $(LIB_SRC:%.c=$(BUILD)/%.o)

.PHONY: all clean test test-bfpp asan
all: $(BUILD)/bfpp

$(BUILD)/bfpp: $(LIB_OBJ) $(BUILD)/src/main.o
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) $^ -o $@

UNIT_MODULES := compiler tape io object program vm
UNIT_BINS := $(addprefix $(BUILD)/test_,$(UNIT_MODULES))
DEPS := $(LIB_OBJ:.o=.d) $(BUILD)/src/main.d $(BUILD)/tests/support/test.d $(addprefix $(BUILD)/tests/unit/test_,$(addsuffix .d,$(UNIT_MODULES)))
.SECONDARY:
-include $(DEPS)

$(BUILD)/test_%: $(LIB_OBJ) $(BUILD)/tests/unit/test_%.o $(BUILD)/tests/support/test.o
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) $^ -o $@

$(BUILD)/%.o: %.c
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(CFLAGS) -MMD -MP -c $< -o $@

.PHONY: test-unit test-cli
test-unit: $(UNIT_BINS)
	@set -e; for test in $(UNIT_BINS); do ./$$test; done

test-cli: $(BUILD)/bfpp
	$(PYTHON) -B tests/test_cli.py ./$(BUILD)/bfpp

test: test-unit test-cli test-bfpp
	$(PYTHON) -B -m unittest discover -s tests -p test_bfpp_runner.py

test-bfpp: $(BUILD)/bfpp
	$(PYTHON) -B tests/run_bfpp_tests.py -- ./$(BUILD)/bfpp

asan:
	$(MAKE) clean
	$(MAKE) CFLAGS='-O1 -g -std=c99 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -fsanitize=address,undefined -fno-omit-frame-pointer' test

clean:
	rm -rf $(BUILD) build build-asan

