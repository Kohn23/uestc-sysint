CC      = gcc
CFLAGS  = -Wall -Wextra -g

BUILD_DIR = build

SOURCES = $(wildcard *.c)
TARGETS = $(addprefix $(BUILD_DIR)/, $(basename $(SOURCES)))

all: $(BUILD_DIR) $(TARGETS)

$(BUILD_DIR):
	mkdir -p $@

$(BUILD_DIR)/%: %.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -rf $(BUILD_DIR)

.PHONY: all clean
