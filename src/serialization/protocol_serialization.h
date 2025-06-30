#ifndef PROTOCOL_SERIALIZATION_H
#define PROTOCOL_SERIALIZATION_H

#include <stdint.h>
#include <string>
#include "server_common.h"

bool read_u32(const uint8_t *&cur, const uint8_t *end, uint32_t &out);
bool read_str(const uint8_t *&cur, const uint8_t *end, size_t n, std::string &out);
int32_t parse_req(const uint8_t *data, size_t size, std::vector<std::string> &out);

#endif