#ifndef SERVER_CONFIG_H
#define SERVER_CONFIG_H

#include <cstddef>

const size_t k_max_msg = 32 << 20;
const size_t k_max_args = 200 * 1000;
const uint64_t k_idle_timeout_ms = 5 * 1000;

#endif