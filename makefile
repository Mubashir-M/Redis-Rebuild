# Define your compiler
CXX = g++

# Define compiler flags
CXXFLAGS = -Wall -Wextra -O2 -g -std=c++17

# Define the build directory for object files
BUILD_DIR = build

# Source directories (where .cpp files reside)
SRC_DIRS = . \
           config \
           connection \
           data \
           data_structures \
           log \
           serialization \
           socket \
           tests \
           utils

# Include directories (where .h files reside for compiler -I flags)
INC_DIRS = $(SRC_DIRS)
CXXFLAGS += $(addprefix -I,$(INC_DIRS))

# --- Source files for each target ---
SERVER_SRCS = 071_server.cpp \
              connection/connection_handlers.cpp \
              data/data_store.cpp \
              data_structures/hashmap.cpp \
              data_structures/hashtable.cpp \
              data_structures/avltree.cpp \
              log/log_utils.cpp \
              serialization/protocol_serialization.cpp \
              socket/socket_utils.cpp \
              utils/buffer_operations.cpp

CLIENT_SRCS = 07_client.cpp

# --- Test source files ---
TEST_AVL_SRCS = tests/test_avl.cpp

# --- Generate object file names for each target ---
# Example: data/data_store.cpp -> build/data/data_store.o
# Note: $(BUILD_DIR)/%.o correctly produces 'build/071_server.o' for '071_server.cpp'
# and 'build/tests/test_avl.o' for 'tests/test_avl.cpp'.
SERVER_OBJS = $(patsubst %.cpp,$(BUILD_DIR)/%.o,$(SERVER_SRCS))
CLIENT_OBJS = $(patsubst %.cpp,$(BUILD_DIR)/%.o,$(CLIENT_SRCS))
TEST_AVL_OBJS = $(patsubst %.cpp,$(BUILD_DIR)/%.o,$(TEST_AVL_SRCS))

# --- Define the executable names ---
SERVER_TARGET = server
CLIENT_TARGET = client
TEST_AVL_TARGET = test_avl

# Define all executables to be built by 'all' target
ALL_EXECUTABLES = $(SERVER_TARGET) $(CLIENT_TARGET) $(TEST_AVL_TARGET)

# List all object files (for cleaning and general purpose)
ALL_OBJS = $(SERVER_OBJS) $(CLIENT_OBJS) $(TEST_AVL_OBJS)

# --- Default target: build all executables ---
all: $(ALL_EXECUTABLES)

# --- Rule to compile .cpp files into .o files ---
# This rule is generic and applies to ALL .cpp files.
# It now directly creates the output directory for the object file.
$(BUILD_DIR)/%.o: %.cpp $(HEADERS)
	@mkdir -p $(@D) # Key change: Create directory for the target object file
	$(CXX) $(CXXFLAGS) -c $< -o $@

# --- SPECIFIC RULE for tests/test_avl.cpp to enable GNU extensions ---
# This rule *overrides* the generic rule above for tests/test_avl.cpp.
# It also creates its specific output directory.
$(BUILD_DIR)/tests/test_avl.o: tests/test_avl.cpp $(HEADERS)
	@mkdir -p $(@D) # Key change: Create directory for this specific object file
	$(CXX) $(CXXFLAGS) -std=gnu++17 -c $< -o $@

# --- Rule to link the SERVER executable ---
$(SERVER_TARGET): $(SERVER_OBJS)
	$(CXX) $(CXXFLAGS) $(SERVER_OBJS) -o $@

# --- Rule to link the CLIENT executable ---
$(CLIENT_TARGET): $(CLIENT_OBJS)
	$(CXX) $(CXXFLAGS) $(CLIENT_OBJS) $(LIBS) -o $@

# --- Rule to link the TEST_AVL executable ---
$(TEST_AVL_TARGET): $(TEST_AVL_OBJS) $(BUILD_DIR)/data_structures/avltree.o \
                    $(BUILD_DIR)/data_structures/hashtable.o \
                    $(BUILD_DIR)/log/log_utils.o \
                    $(BUILD_DIR)/utils/buffer_operations.o
	$(CXX) $(CXXFLAGS) $^ -o $@

# Clean up compiled files and executables
clean:
	rm -rf $(BUILD_DIR) $(ALL_EXECUTABLES)

.PHONY: all clean $(ALL_EXECUTABLES)