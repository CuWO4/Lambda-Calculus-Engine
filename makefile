MAKEFLAGS += -j20

FB_EXT := .cpp

# Flags
CXXFLAGS := -Wall -Wno-register -std=c++17
FFLAGS :=
BFLAGS := -d
LDFLAGS :=

# Debug flags
DEBUG ?= 0
ifeq ($(DEBUG), 0)
CXXFLAGS += -O2
else
CXXFLAGS += -g -O0
endif

# Profile flag
PROFILE ?= 0
ifeq ($(PROFILE), 0)
PROFILEFLAG :=
else
PROFILEFLAG := -pg
endif

# Compilers
CXX := clang++
FLEX := flex
BISON := bison

# Directories
TARGET_EXEC := lambda
SRC_DIR := src
BUILD_DIR ?= build
INC_DIR ?= $(CDE_INCLUDE_PATH)

# Source files & target files
FB_SRCS := $(patsubst $(SRC_DIR)/%.l, $(BUILD_DIR)/%.lex$(FB_EXT), $(wildcard $(SRC_DIR)/*.l))
FB_SRCS += $(patsubst $(SRC_DIR)/%.y, $(BUILD_DIR)/%.tab$(FB_EXT), $(wildcard $(SRC_DIR)/*.y))
SRCS := $(FB_SRCS) $(wildcard $(SRC_DIR)/*.cpp)
OBJS := $(patsubst $(SRC_DIR)/%.cpp, $(BUILD_DIR)/%.cpp.o, $(SRCS))
OBJS := $(patsubst $(BUILD_DIR)/%.cpp, $(BUILD_DIR)/%.cpp.o, $(OBJS))

# Header directories & dependencies
INC_DIRS := include/
INC_FLAGS := $(addprefix -I, $(INC_DIRS))
CPPFLAGS = $(INC_FLAGS) -MMD -MP

# Main target
$(BUILD_DIR)/$(TARGET_EXEC): $(FB_SRCS) $(OBJS)
	$(CXX) $(OBJS) $(LDFLAGS) $(PROFILEFLAG) -o $@

# C++ source
define cxx_recipe
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(PROFILEFLAG) -c $< -o $@
endef
$(BUILD_DIR)/%.cpp.o: $(SRC_DIR)/%.cpp | $(BUILD_DIR); $(cxx_recipe)
$(BUILD_DIR)/%.cpp.o: $(BUILD_DIR)/%.cpp | $(BUILD_DIR); $(cxx_recipe)

# Flex
$(BUILD_DIR)/%.lex$(FB_EXT): $(SRC_DIR)/%.l | $(BUILD_DIR)
	$(FLEX) -o$@ $(FFLAGS) $<

# Bison
$(BUILD_DIR)/%.tab$(FB_EXT): $(SRC_DIR)/%.y | $(BUILD_DIR)
	$(BISON) $(BFLAGS) -o $@ $<

$(BUILD_DIR) :
	mkdir "$(BUILD_DIR)"

DEPS := $(OBJS:.o=.d)
-include $(DEPS)

.PHONY: clean

clean:
	-rm -rf $(BUILD_DIR)

test: $(BUILD_DIR)/$(TARGET_EXEC)
	./$(BUILD_DIR)/$(TARGET_EXEC) lib/test.lambda -o lib/test.out