#ifndef BUFFER_OPERATIONS_H
#define BUFFER_OPERATIONS_H

#include <vector>

void buf_append(std::vector<uint8_t> &buf, const uint8_t *data, size_t len);
void buf_consume(std::vector<uint8_t> &buf, size_t n);
#endif