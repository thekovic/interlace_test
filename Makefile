BUILD_DIR=build
include $(N64_INST)/include/n64.mk

# A for NTSC, P for PAL
N64_ROM_REGION="P"

src = main.c

all: interlace_test.z64

interlace_test.z64: N64_ROM_TITLE="Interlace Test"
interlace_test.z64: $(BUILD_DIR)/interlace_test.dfs

filesystem/dummy.txt:
	echo "dfs" > filesystem/dummy.txt

$(BUILD_DIR)/interlace_test.dfs: $(wildcard filesystem/*) filesystem/dummy.txt
$(BUILD_DIR)/interlace_test.elf: $(src:%.c=$(BUILD_DIR)/%.o) 

clean:
	rm -rf $(BUILD_DIR) interlace_test.z64

-include $(wildcard $(BUILD_DIR)/*.d)

.PHONY: all clean
