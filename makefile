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
              log/log_utils.cpp \
              serialization/protocol_serialization.cpp \
              socket/socket_utils.cpp \
              utils/buffer_operations.cpp

CLIENT_SRCS = 07_client.cpp

# --- Generate object file names for each target ---
# Example: data/data_store.cpp -> build/data/data_store.o
SERVER_OBJS = $(patsubst %.cpp,$(BUILD_DIR)/%.o,$(SERVER_SRCS))
CLIENT_OBJS = $(patsubst %.cpp,$(BUILD_DIR)/%.o,$(CLIENT_SRCS))

# --- Define the executable names ---
SERVER_TARGET = server
CLIENT_TARGET = client

# Define all executables to be built by 'all' target
ALL_EXECUTABLES = $(SERVER_TARGET) $(CLIENT_TARGET)

# List all object files (for cleaning and general purpose)
ALL_OBJS = $(SERVER_OBJS) $(CLIENT_OBJS)

# --- Default target: build all executables ---
all: $(ALL_EXECUTABLES)

# --- Rule to create all output directories ---
# This generates a list of all unique directories where .o files will go (e.g., build/, build/data/, etc.)
# Each object file will implicitly depend on its specific directory.
$(BUILD_DIR)/%.o: $(BUILD_DIR)/%/.created_dir # This is a placeholder for the directory creation dependency
.SECONDARY: $(BUILD_DIR)/%/.created_dir # Mark as secondary so make doesn't delete them if their parent doesn't exist.

# Generic rule to create any directory where an object file will reside
# E.g., build/data/.created_dir will ensure build/data exists
$(BUILD_DIR)/%/.created_dir:
	mkdir -p $(dir $@)

# --- Rule to compile .cpp files into .o files ---
# Each .o file depends on its source .cpp, headers, and its specific output directory.
# The directory dependency ensures mkdir -p runs BEFORE compilation for each .o
$(BUILD_DIR)/%.o: %.cpp $(HEADERS) | $(BUILD_DIR)/%/.created_dir # Order-only prerequisite for specific dir
	$(CXX) $(CXXFLAGS) -c $< -o $@

# --- Rule to link the SERVER executable ---
$(SERVER_TARGET): $(SERVER_OBJS)
	$(CXX) $(CXXFLAGS) $(SERVER_OBJS) -o $@

# --- Rule to link the CLIENT executable ---
$(CLIENT_TARGET): $(CLIENT_OBJS)
	$(CXX) $(CXXFLAGS) $(CLIENT_OBJS) $(LIBS) -o $@ # Add LIBS if client needs external libraries

# Clean up compiled files and executables
clean:
	rm -rf $(BUILD_DIR) $(ALL_EXECUTABLES)

.PHONY: all clean