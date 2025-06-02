#include "data_store.h"
std::map<std::string, std::string> g_data;

void do_request(std::vector<std::string> &cmd, Response &out){
    if (cmd.size() == 2 && cmd[0] == "get"){
        auto it = g_data.find(cmd[1]);
        if (it == g_data.end()){
            out.status = RES_NX;
            return;
        }
        out.value_ptr = &it->second;
    } else if (cmd.size() == 3 && cmd[0] == "set"){
        g_data[cmd[1]].swap(cmd[2]);
    } else if (cmd.size() == 2 && cmd[0] == "del"){
        g_data.erase(cmd[1]);
    } else {
        out.status = RES_ERR;
    }
};