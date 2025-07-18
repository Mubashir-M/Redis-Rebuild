# Define your compiler
CXX = g++

# This instructs the compiler (g++) to target x86_64 architecture
# when running on an ARM-based M1 Mac.
ARCH_FLAG = -target x86_64-apple-darwin

# Define compiler flags
CXXFLAGS = -Wall -Wextra -O0 -g -std=c++17 $(ARCH_FLAG)

# Define the build directory for object files
BUILD_DIR = build

# Source directories (where .cpp files reside)
SRC_DIRS = src \
           src/config \
           src/connection \
           src/data \
           src/data_structures \
           src/log \
           src/serialization \
           src/socket \
           src/threads \
           src/utils \
           tests

# Include directories (where .h files reside for compiler -I flags)
INC_DIRS = $(SRC_DIRS)
CXXFLAGS += $(addprefix -I,$(INC_DIRS))

# --- Source files for each target ---
SERVER_SRCS = src/server.cpp \
              src/connection/connection_handlers.cpp \
              src/data/data_store.cpp \
              src/data_structures/hashmap.cpp \
              src/data_structures/hashtable.cpp \
              src/data_structures/avltree.cpp \
              src/data_structures/zset.cpp \
              src/data_structures/heap.cpp \
              src/log/log_utils.cpp \
              src/serialization/protocol_serialization.cpp \
              src/socket/socket_utils.cpp \
              src/utils/buffer_operations.cpp \
              src/threads/thread_pool.cpp \
              src/utils/timer.cpp

CLIENT_SRCS = src/client.cpp

# --- Test source files ---
TEST_AVL_SRCS = tests/test_avl.cpp
TEST_OFFSET_SRCS = tests/test_offset.cpp

# --- Generate object file names for each target ---
SERVER_OBJS = $(patsubst %.cpp,$(BUILD_DIR)/%.o,$(SERVER_SRCS))
CLIENT_OBJS = $(patsubst %.cpp,$(BUILD_DIR)/%.o,$(CLIENT_SRCS))
TEST_AVL_OBJS = $(patsubst %.cpp,$(BUILD_DIR)/%.o,$(TEST_AVL_SRCS))
TEST_OFFSET_OBJS = $(patsubst %.cpp,$(BUILD_DIR)/%.o,$(TEST_OFFSET_SRCS))

# --- Define the executable names ---
SERVER_TARGET = server
CLIENT_TARGET = client
TEST_AVL_TARGET = test_avl
TEST_OFFSET_TARGET = test_offset

# Define all executables to be built by 'all' target
ALL_EXECUTABLES = $(SERVER_TARGET) $(CLIENT_TARGET) $(TEST_AVL_TARGET) $(TEST_OFFSET_TARGET)

# List all object files (for cleaning and general purpose)
ALL_OBJS = $(SERVER_OBJS) $(CLIENT_OBJS) $(TEST_AVL_OBJS) $(TEST_OFFSET_OBJS)

# --- Default target: build all executables ---
all: $(ALL_EXECUTABLES)

# --- Rule to compile .cpp files into .o files ---
# This rule is generic and applies to ALL .cpp files.
# It now directly creates the output directory for the object file.
$(BUILD_DIR)/%.o: %.cpp $(HEADERS)
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# --- SPECIFIC RULE for tests/test_avl.cpp to enable GNU extensions ---
# This rule *overrides* the generic rule above for tests/test_avl.cpp.
# It also creates its specific output directory.
$(BUILD_DIR)/tests/test_avl.o: tests/test_avl.cpp $(HEADERS)
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) -std=gnu++17 -c $< -o $@

$(BUILD_DIR)/tests/test_offset.o: tests/test_offset.cpp $(HEADERS)
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) -std=gnu++17 -c $< -o $@

# --- Rule to link the SERVER executable ---
$(SERVER_TARGET): $(SERVER_OBJS)
	$(CXX) $(CXXFLAGS) $(SERVER_OBJS) -o $@

# --- Rule to link the CLIENT executable ---
$(CLIENT_TARGET): $(CLIENT_OBJS)
	$(CXX) $(CXXFLAGS) $(CLIENT_OBJS) $(LIBS) -o $@

# --- Rule to link the TEST_AVL executable ---
$(TEST_AVL_TARGET): $(TEST_AVL_OBJS) $(BUILD_DIR)/src/data_structures/avltree.o \
                    $(BUILD_DIR)/src/data_structures/hashtable.o \
                    $(BUILD_DIR)/src/log/log_utils.o \
                    $(BUILD_DIR)/src/utils/buffer_operations.o
	$(CXX) $(CXXFLAGS) $^ -o $@

$(TEST_OFFSET_TARGET): $(TEST_OFFSET_OBJS) $(BUILD_DIR)/src/data_structures/zset.o \
                       $(BUILD_DIR)/src/data_structures/avltree.o \
                       $(BUILD_DIR)/src/data_structures/hashtable.o \
                       $(BUILD_DIR)/src/data_structures/hashmap.o \
                       $(BUILD_DIR)/src/log/log_utils.o \
                       $(BUILD_DIR)/src/utils/buffer_operations.o
	$(CXX) $(CXXFLAGS) $^ -o $@

# Clean up compiled files and executables
clean:
	rm -rf $(BUILD_DIR) $(ALL_EXECUTABLES)

.PHONY: all clean $(ALL_EXECUTABLES)