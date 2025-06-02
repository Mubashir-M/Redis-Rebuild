# Define your compiler
CXX = g++

# Define compiler flags
CXXFLAGS = -Wall -Wextra -O2 -g -std=c++17

# List your header files (for dependency tracking, optional but good practice)
HEADERS = buffer_operations.h connection_handlers.h data_store.h \
          log_utils.h protocol_serialization.h server_common.h \
          server_config.h socket_utils.h

# List all your source files
SRCS = 071_server.cpp \
       buffer_operations.cpp \
       connection_handlers.cpp \
       data_store.cpp \
       log_utils.cpp \
       protocol_serialization.cpp \
       socket_utils.cpp

# Define the build directory for object files
BUILD_DIR = build

# Generate object file names from source file names
OBJS = $(addprefix $(BUILD_DIR)/, $(SRCS:.cpp=.o))

# Define the executable name
TARGET = server

# Default target: build the server
all: $(BUILD_DIR) $(TARGET)

# Rule to create build directory if it does not exist
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)
# Rule to link the executable
$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) $(OBJS) -o $@

# Rule to compile .cpp files into .o files
# $@ is the target, $< is the first prerequisite
$(BUILD_DIR)/%.o: %.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean up compiled files
clean:
	rm -rf $(BUILD_DIR) $(TARGET)

.PHONY: all clean $(BUILD_DIR)