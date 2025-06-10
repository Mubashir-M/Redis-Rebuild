#ifndef CONNECTION_HANDLERS_H
#define CONNECTION_HANDLERS_H

#include "server_common.h"

void handle_write(Conn *conn);
bool try_one_request(Conn *conn);
void handle_read(Conn *conn);
Conn* handle_accept(int fd);
#endif