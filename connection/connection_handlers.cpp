#include "connection_handlers.h"
#include "socket_utils.h"
#include "buffer_operations.h"
#include "log_utils.h"
#include "server_config.h"
#include "protocol_serialization.h"
#include "data_store.h"

#include <assert.h>

#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <unistd.h>

bool try_one_request(Conn *conn){
    // check incoming size
    if(conn->incoming.size() < 4){
        return false;
    }

    //copy len of incoming data
    uint32_t len = 0;
    memcpy(&len, conn->incoming.data(), 4);
    if(len > k_max_msg){
        msg("too long");
        conn->want_close = true;
        return false;
    }

    if(4 + len > conn->incoming.size()){
        return false; // continue to read
    }

    const uint8_t *request = &conn->incoming[4];

    std::vector<std::string> cmd;
    if(parse_req(request,len,cmd) < 0){
        msg("bad request");
        conn->want_close = true;
        return false;
    }

    Response resp;
    do_request(cmd, resp);
    make_response(resp, conn->outgoing);

    buf_consume(conn->incoming, 4 +len);
    return true;
};

void handle_write(Conn *conn){
    // check ooutgoin size > 0
    assert(conn->outgoing.size() > 0);
    // write to network
    int rv = write(conn->fd, conn->outgoing.data(), conn->outgoing.size());
    // check if written 2
    if(rv < 0 && errno == EAGAIN){
        return;
    }

    if(rv < 0){
        msg("write() error");
        return;
    }

    //remove written from outgoing
    buf_consume(conn->outgoing, size_t(rv));
    // update readiness
    if(conn->outgoing.size() == 0){
        conn->want_read = true;
        conn->want_write = false;
    }

};

void handle_read(Conn *conn){
    uint8_t buf[64*1024];
    ssize_t rv = read(conn->fd, buf, sizeof(buf));
    if(rv < 0 && errno == EAGAIN){
        return; // not read
    }

    if (rv < 0){ // IO error
        msg_errno("read() error");
        conn->want_close = true;
        return;
    }

    if(rv == 0){// EOF
        if(conn->incoming.size() == 0){
            msg("client closed");
        } else {
            msg("Unexpected EOF");
        }
        conn->want_close = true;
        return;
    }

    // add data from buffer to Conn::incoming.
    buf_append(conn->incoming, buf, (size_t)rv);

    // parse requests and generate reponses
    while(try_one_request(conn)){}

    //update readiness
    if(conn->outgoing.size() > 0){
        conn->want_read = false;
        conn->want_write = true;
        return handle_write(conn);
    }

};


Conn* handle_accept(int fd){
    struct sockaddr_in client_addr = {};
    socklen_t addrlen = sizeof(client_addr);
    int connfd = accept(fd, (struct sockaddr *)&client_addr, &addrlen);
    if (connfd <  0){
        msg_errno("accept() error");
        return NULL;
    }
    uint32_t ip  = client_addr.sin_addr.s_addr;
    fprintf(stderr, "new client from %u.%u.%u.%u.%u\n",
        ip & 255, (ip >> 8) & 255, (ip >> 16) & 255, ip >> 24,
    ntohs(client_addr.sin_port)
    );

    fd_set_nb(connfd);

    Conn *conn = new Conn();
    conn->fd = connfd;
    conn->want_read = true;
    return conn;
};