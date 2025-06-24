#ifndef BUFFER_OPERATIONS_H
#define BUFFER_OPERATIONS_H

#include "server_common.h"
#include <vector>

void buf_append(std::vector<uint8_t> &buf, const uint8_t *data, size_t len);
void buf_consume(std::vector<uint8_t> &buf, size_t n);
void buf_append_u8(Buffer &buf, uint8_t data);
void buf_append_u32(Buffer &buf, uint32_t data);
void buf_append_i64(Buffer &buf, uint64_t data);
void buf_append_dbl(Buffer &buf, double data);
#endif