#ifndef DATA_STORE_H
#define DATA_STORE_H

#include "server_common.h"

#include <map>
#include <string>

extern std::map<std::string, std::string> g_data;
void do_request(std::vector<std::string> &cmd, Response &out);

#endif