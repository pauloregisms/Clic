CC ?= cc
CPPFLAGS ?=
CFLAGS ?= -std=gnu11 -Wall -Wextra
LDFLAGS ?=
LDLIBS ?=
BUILD_DIR ?= build
EXAMPLE_SOURCES := $(wildcard exemplos/*.c exemplos/rede/*.c)
EXAMPLE_BINARIES := $(patsubst exemplos/%.c,$(BUILD_DIR)/%,$(EXAMPLE_SOURCES))

.PHONY: examples clean

examples: $(EXAMPLE_BINARIES)

$(BUILD_DIR):
	mkdir -p $@

$(BUILD_DIR)/%: exemplos/%.c Clic.c Clic.h | $(BUILD_DIR)
	mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(CFLAGS) -I. $< Clic.c $(LDFLAGS) $(LDLIBS) -o $@

clean:
	rm -rf $(BUILD_DIR)

.PHONY: test test-sanitize

$(BUILD_DIR)/test_clic: tests/test_clic.c Clic.c Clic.h | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -Werror -UNDEBUG -I. $< $(LDFLAGS) $(LDLIBS) -o $@

test: $(BUILD_DIR)/test_clic examples
	./$(BUILD_DIR)/test_clic

# Separate output directory prevents reusing uninstrumented binaries.
test-sanitize:
	$(MAKE) BUILD_DIR=$(BUILD_DIR)/sanitize CFLAGS="-std=gnu11 -Wall -Wextra -g -O1 -fsanitize=address,undefined -fno-omit-frame-pointer -fno-pie" LDFLAGS="-fsanitize=address,undefined -no-pie" test

# Requires Python 3 and the pre-networking commit in local Git history.
.PHONY: test-legacy
test-legacy:
	python3 tests/test_legacy.py
