#include <sys/socket.h>
#include <netinet/in.h>

#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <iostream> // For std::cout, std::cerr, std::cin
#include <vector>
#include <string>
#include <sstream>
#include <signal.h>
#include <cstdint> // For uint64_t, uint32_t, int32_t

// Helper function to print error messages and abort
static void die(const char *msg) {
    int err = errno; // Capture errno before any other calls might change it
    fprintf(stderr, "[%d] %s\n", err, msg);
    abort(); // Terminate the program immediately
}

// Helper function to print informational messages to stderr
static void msg(const char *msg) {
    fprintf(stderr, "%s\n", msg);
}

// Reads 'n' bytes fully from file descriptor 'fd' into 'buf'.
// Returns 0 on success, -1 on EOF or error.
static int32_t read_full(int fd, char *buf, size_t n){
    while (n > 0){
        ssize_t rv = read(fd, buf, n);
        if (rv <= 0){
            // rv == 0 indicates EOF, rv < 0 indicates an error
            return -1;
        }
        assert((size_t)rv <= n); // Ensure we don't read more than requested
        n -= (size_t)rv; // Decrement remaining bytes
        buf += rv;       // Advance buffer pointer
    }
    return 0; // All bytes read successfully
}

// Writes 'n' bytes fully from 'buf' to file descriptor 'fd'.
// Returns 0 on success, -1 on error.
static int32_t write_all(int fd, const char *buf, size_t n){
    while(n > 0){
        ssize_t rv = write(fd, buf, n);
        if (rv <= 0){
            // rv == 0 indicates EOF (unlikely for write), rv < 0 indicates an error
            return -1;
        }
        assert((size_t)rv <= n); // Ensure we don't write more than requested
        n -= (size_t)rv; // Decrement remaining bytes
        buf += rv;       // Advance buffer pointer
    }
    return 0; // All bytes written successfully
}

const size_t k_max_msg = 4096; // Maximum size for a message payload

// Tags for different response types (similar to Redis RESP)
enum {
    TAG_NIL = 0, // Null response
    TAG_ERR = 1, // Error response
    TAG_STR = 2, // String response
    TAG_INT = 3, // Integer response
    TAG_DBL = 4, // Double response
    TAG_ARR = 5, // Array response
};

// Sends a command request to the server.
// 'cmd' is a vector of strings representing the command and its arguments.
// The request format is: total_length (4 bytes) | num_args (4 bytes) |
//                        arg1_len (4 bytes) | arg1_data |
//                        arg2_len (4 bytes) | arg2_data | ...
// Returns 0 on success, -1 if the request is too large or write fails.
static int32_t send_req(int fd, const std::vector<std::string> &cmd){
    // Calculate total length of the payload
    uint32_t len = 4; // For num_args
    for (const std::string &s : cmd){
        len += 4 + (uint32_t)s.size(); // For arg_len + arg_data
    }

    // Check if the total message length exceeds the maximum allowed
    if (len > k_max_msg){
        return -1;
    }

    // Prepare the write buffer
    char wbuf[4 + k_max_msg]; // 4 bytes for total_length + max_msg payload
    memcpy(&wbuf[0], &len, 4); // Copy total_length to the beginning of the buffer

    uint32_t n = (uint32_t)cmd.size();
    memcpy(&wbuf[4], &n, 4); // Copy num_args after total_length
    size_t cur = 8;          // Current position in wbuf (after total_length and num_args)

    // Copy each argument's length and data into the buffer
    for(const std::string &s : cmd){
        uint32_t p = (uint32_t)s.size();
        memcpy(&wbuf[cur], &p, 4); // Copy arg_len
        memcpy(&wbuf[cur + 4], s.data(), s.size()); // Copy arg_data
        cur += 4 + s.size(); // Advance current position
    }
    // Write the entire prepared buffer to the socket
    return write_all(fd, wbuf, 4 + len);
}

// Prints the server response to stdout based on its tag.
// 'data' points to the raw response payload, 'size' is its length.
// Returns the number of bytes consumed from 'data' on success, -1 on error.
static int32_t print_response(const uint8_t *data, size_t size){
    if (size < 1) {
        msg("bad response: too short");
        return -1;
    }

    // Process response based on its tag
    switch(data[0]){
        case TAG_NIL:
            printf("(nil)\n");
            return 1; // Tag byte consumed
        case TAG_ERR: {
            if (size < 1 + 8) { // Tag + code (4) + len (4)
                msg("bad response: ERR too short");
                return -1;
            }
            int32_t code = 0;
            uint32_t len = 0;
            memcpy(&code, &data[1], 4);     // Read error code
            memcpy(&len, &data[1 + 4], 4);  // Read error message length
            if (size < 1 + 8 + len) { // Check if message data is truncated
                msg("bad response: ERR message truncated");
                return -1;
            }
            printf("(err) %d %.*s\n", code, len, &data[1 + 8]); // Print error code and message
            return (int32_t)(1 + 8 + len); // Bytes consumed: tag + code + len + message data
        }
        case TAG_STR: {
            if (size < 1 + 4) { // Tag + len (4)
                msg("bad response: STR too short");
                return -1;
            }
            uint32_t len = 0;
            memcpy(&len, &data[1], 4); // Read string length
            if (size < 1 + 4 + len) { // Check if string data is truncated
                msg("bad response: STR truncated");
                return -1;
            }
            printf("(str) %.*s\n", len, &data[1 + 4]); // Print string (using precision for length)
            return (int32_t)(1 + 4 + len); // Bytes consumed: tag + len + string data
        }
        case TAG_INT: {
            if (size < 1 + 8) { // Tag + int64_t (8)
                msg("bad response: INT too short");
                return -1;
            }
            int64_t val = 0;
            memcpy(&val, &data[1], 8); // Read 64-bit integer
            printf("(int) %lld\n", val); // Print as long long
            return 1 + 8; // Bytes consumed: tag + int64_t
        }
        case TAG_DBL: {
            if (size < 1 + 8) { // Tag + double (8)
                msg("bad response: DBL too short");
                return -1;
            }
            double val = 0;
            memcpy(&val, &data[1], 8); // Read double
            printf("(dbl) %g\n", val); // Print as double
            return 1 + 8; // Bytes consumed: tag + double
        }
        case TAG_ARR: {
            if (size < 1 + 4) { // Tag + len (4)
                msg("bad response: ARR too short");
                return -1;
            }
            uint32_t len = 0;
            memcpy(&len, &data[1], 4); // Read array length
            printf("(arr) len=%u\n", len); // Print array length
            size_t arr_bytes = 1 + 4; // Bytes consumed so far: tag + array length
            for (uint32_t i = 0; i < len; ++i) {
                // Recursively print each element of the array
                int32_t rv_elem = print_response(&data[arr_bytes], size - arr_bytes);
                if (rv_elem < 0) {
                    return rv_elem; // Error in printing element
                }
                arr_bytes += (size_t)rv_elem; // Accumulate bytes consumed by elements
            }
            printf("(arr) end\n"); // Print array end marker
            return (int32_t)arr_bytes; // Total bytes consumed by array and its elements
        }
        default:
            msg("bad response: unknown tag");
            return -1;
    }
}

// Reads a response from the server.
// The response format is: total_length (4 bytes) | payload (variable length)
// Returns 0 on success, -1 on error.
static int32_t read_res(int fd){
    char rbuf[4 + k_max_msg]; // 4 bytes for total_length + max_msg payload
    errno = 0; // Clear errno before read operation

    // Read the 4-byte total length header
    int32_t err = read_full(fd, rbuf, 4);
    if (err){
        // If read_full returns -1, check errno for specific error or EOF
        msg(errno == 0 ? "EOF" : "read() error on length header");
        return err;
    }

    uint32_t len = 0;
    memcpy(&len, rbuf, 4); // Extract the total length
    if (len > k_max_msg){
        msg("response payload too long");
        return -1;
    }

    // Read the remaining payload based on the received length
    err = read_full(fd, &rbuf[4], len);
    if (err) {
        msg("read() error on payload");
        return err;
    }

    // Print the response payload
    int32_t rv = print_response((uint8_t *)&rbuf[4], len);
    // Verify that print_response consumed exactly the expected payload length
    if (rv > 0 && (uint32_t)rv != len) {
        msg("bad response: bytes mismatch after printing");
        rv = -1;
    }
    return rv < 0 ? rv : 0; // Return 0 on success, -1 on error
}

int main(){
    signal(SIGPIPE, SIG_IGN); // Ignore SIGPIPE to prevent client crash on broken pipe

    // 1. Create a socket
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0 ){
        die("socket()"); // Abort if socket creation fails
    }

    // 2. Prepare server address structure
    struct sockaddr_in addr = {};
    addr.sin_family = AF_INET;          // IPv4
    addr.sin_port = ntohs(1234);        // Server port (network byte order)
    addr.sin_addr.s_addr = ntohl(INADDR_LOOPBACK); // Loopback address (127.0.0.1)

    // 3. Connect to the server
    int rv = connect(fd, (const struct sockaddr *)&addr, sizeof(addr));
    if(rv){
        die("connect()"); // Abort if connection fails
    }

    // Initial connection message to stderr
    std::cerr << "Connected to server on 127.0.0.1:1234. Type 'quit' to exit.\n";

    // Main interactive loop
    while (true) {
        std::cerr << "> ";
        std::string line;
        // Read a line from standard input (stdin)
        std::getline(std::cin, line);

        // Check for EOF on stdin (e.g., pipe closed by Python test script)
        if (std::cin.eof()) {
            std::cerr << "EOF detected. Disconnecting.\n";
            break; // Exit loop if EOF
        }

        // Handle empty input lines
        if (line.empty()) {
            std::cerr << "> "; // Print prompt again for empty line
            std::cerr.flush();   // Flush stderr
            continue; // Continue to next loop iteration
        }

        // Parse command and arguments
        std::vector<std::string> cmd;
        std::istringstream iss(line);
        std::string token;
        while (iss >> token) {
            cmd.push_back(token);
        }

        // Handle 'quit' command
        if (!cmd.empty() && cmd[0] == "quit") {
            std::cerr << "Disconnecting.\n";
            break; // Exit loop for quit command
        }

        // Send the command request to the server
        int32_t err = send_req(fd, cmd);
        if (err){
            msg("Failed to send request. Disconnecting.");
            break; // Exit loop on send error
        }

        // Read and print the server's response
        err = read_res(fd);
        if (err){
            msg("Failed to read response. Disconnecting.");
            break; // Exit loop on read error
        }
    }

    close(fd); // Close the socket file descriptor
    return 0;  // Exit successfully
}
