CC = gcc
CFLAGS = -Wall -Wextra -g -I./include
LDFLAGS = 

SRC_DIR = src
BUILD_DIR = build
INCLUDE_DIR = include
EXAMPLES_DIR = examples
TEST_DIR = tests

# 源文件和目标文件
SRCS = $(wildcard $(SRC_DIR)/*.c)
OBJS = $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(SRCS))
LIB = $(BUILD_DIR)/libphymuti.a

# 示例程序
EXAMPLE_SRCS = $(wildcard $(EXAMPLES_DIR)/*.c)
EXAMPLE_BINS = $(patsubst $(EXAMPLES_DIR)/%.c,$(BUILD_DIR)/examples/%,$(EXAMPLE_SRCS))

# 测试程序
TEST_SRCS = $(wildcard $(TEST_DIR)/*.c)
TEST_BINS = $(patsubst $(TEST_DIR)/%.c,$(BUILD_DIR)/tests/%,$(TEST_SRCS))

# 默认目标
all: directories $(LIB) examples tests

# 创建构建目录
directories:
	@mkdir -p $(BUILD_DIR)
	@mkdir -p $(BUILD_DIR)/examples
	@mkdir -p $(BUILD_DIR)/tests

# 构建静态库
$(LIB): $(OBJS)
	ar rcs $@ $^

# 编译源文件
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

# 构建示例程序
examples: $(EXAMPLE_BINS)

$(BUILD_DIR)/examples/%: $(EXAMPLES_DIR)/%.c $(LIB)
	$(CC) $(CFLAGS) $< -o $@ $(LIB) $(LDFLAGS)

# 构建测试程序
tests: $(TEST_BINS)

$(BUILD_DIR)/tests/%: $(TEST_DIR)/%.c $(LIB)
	$(CC) $(CFLAGS) $< -o $@ $(LIB) $(LDFLAGS)

# 运行测试
run_tests: tests
	@for test in $(TEST_BINS); do \
		echo "Running $$test"; \
		$$test; \
	done

# 清理构建文件
clean:
	rm -rf $(BUILD_DIR)

.PHONY: all directories examples tests run_tests clean 