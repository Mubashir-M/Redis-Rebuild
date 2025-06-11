#include "buffer_operations.h"


void buf_append(std::vector<uint8_t> &buf, const uint8_t *data, size_t len){
    buf.insert(buf.end(), data, data + len);
}

void buf_consume(std::vector<uint8_t> &buf, size_t n){
    buf.erase(buf.begin(), buf.begin() + n);
}

void buf_append_u8(Buffer &buf, uint8_t data){
    buf.push_back(data);
};

void buf_append_u32(Buffer &buf, uint32_t data){
    buf_append(buf, (const uint8_t *)&data, 4);
};

void buf_append_i64(Buffer &buf, uint64_t data){
    buf_append(buf, (const uint8_t *)&data, 8);
};