# Redis-Rebuild

Redis Rebuild
This project is a simplified re-implementation of a Redis-like server in C++, focusing on understanding core components and data structures used in high-performance network services. It includes a basic server, client, and various data structure implementations.

## Directory Structure

The project is organized into logical directories for better modularity and maintainability:

```
.
├── README.md                 // This file
├── makefile                  // Project build instructions
├── src/                      // Core source code for the server and client
│   ├── client.cpp            // Simple client application
│   ├── server.cpp            // The main server application
│   ├── config/               // Configuration headers (e.g., common constants)
│   ├── connection/           // Network connection handling logic
│   ├── data/                 // Data storage and management (main database)
│   ├── data_structures/      // Implementations of various data structures (hashmap, avltree, heap, dlist, zset)
│   ├── log/                  // Logging utilities
│   ├── serialization/        // Protocol serialization/deserialization (RESP-like)
│   ├── socket/               // Socket utilities (non-blocking, etc.)
│   ├── threads/              // Thread pool implementation
│   └── utils/                // General utilities (buffer operations, timer)
└── tests/                    // Unit tests for data structures
    ├── test_avl.cpp          // Test for AVL tree
    └── test_offset.cpp       // Test for offset-related data structures (e.g., zset)
```

## Features

- **Basic Key-Value Store:** Supports fundamental Redis-like commands.

- **Idle Connection Timeout:** Automatically closes inactive client connections.

- **TTL Cache Expiration:** Implements Time-To-Live for cached items, automatically expiring them.

- **Event-Driven I/O:** Uses `poll()`for efficient handling of multiple client connections.

- **Custom Data Structures:** Implements various data structures from scratch (Hash Map, AVL Tree, Doubly Linked List, Min-Heap, Sorted Set).

## Building the Project

To build the server, client, and test executables, navigate to the root directory of the project and use make.

1. **Clean previous builds (recommended before a fresh build):**

   ```
   make clean
   ```

2. **Build all executables:**

   ```
   make
   ```

This will create executables (`server`, ´client´, `test_avl`, `test_offset`) in the project root directory.

## Running the Server

The server listens on `127.0.0.1` (localhost) on port `1234` by default.

1. **Open a terminal.**

2. **Navigate to the project root.**

3. **Run the server:**

   ```
   ./server
   ```

The server will start and print log messages to the console. Keep this terminal open.

# Running the Client

You can interact with the server using the provided C++ client or a tool like `socat`.

### Using the C++ Client

1. **Open a new terminal.**

2. **Navigate to the project root.**

3. **Run the client:**

```
./client
```

You can then type Redis-like commands (e.g., `SET mykey myvalue`, `GET mykey`, `PEXPIRE mykey 5000`, `PTTL mykey`).

### Using socat (for Manual Testing)

`socat` is a versatile command-line tool for relaying data. It's useful for quick manual tests or observing raw protocol interactions.

- **Installation:** If you don't have socat, install it via Homebrew on macOS:
  ```
  brew install socat
  ```
- **Connect to the server:**
  ```
  socat tcp:127.0.0.1:1234 -
  ```
  You can then type commands directly. For instance, type `SET nmykey nmyvalue` and press Enter.

## Running Tests

### Data Structure Unit Tests

You can run the individual data structure tests:

1. **Navigate to the project root.**

2. **Run AVL Tree tests:**
   ```
   ./test_avl
   ```
3. **Run Offset tests (e.g., ZSET):**
   ```
   ./test_offset
   ```
