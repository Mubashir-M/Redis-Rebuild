#include "buffer_operations.h"
#include "connection_handlers.h"
#include "data_store.h"
#include "log_utils.h"
#include "protocol_serialization.h"
#include "server_common.h"
#include "server_config.h"
#include "socket_utils.h"

#include <sys/socket.h>
#include <poll.h>
#include <assert.h>
#include <unistd.h>
#include <netinet/in.h>

int main(){

    int fd = socket(AF_INET, SOCK_STREAM, 0);

    if (fd == -1 ){
        die("socket()");
    }

    int val = 1;

    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val)) == -1 ){
        die("setsockopt()");
    }

    struct sockaddr_in addr = {};

    addr.sin_family = AF_INET;
    addr.sin_port = ntohs(1234);
    addr.sin_addr.s_addr = ntohl(0);

    int rv = bind(fd, (const sockaddr *)&addr, sizeof(addr));

    if (rv){
        die("bind()");
    }

    // set the listen fd to nonblocking mode
    fd_set_nb(fd);

    // listen
    rv = listen(fd, SOMAXCONN);
    if(rv){
        die("listen()");
    }
    
    // map of all client connections
    std::vector<Conn *> fd2conn;

    // the event loop
    std::vector<struct pollfd> poll_args;

    while(true){
        //prepare the argument for the poll()
        poll_args.clear();
        // put the listening socket in the first position
        struct pollfd pfd = {fd, POLLIN, 0}; //POLLIN or POLLIN
        poll_args.push_back(pfd);
        // the rest are connection sockets
        for (Conn *conn : fd2conn){
            if(!conn){
                continue;
            }

            struct pollfd pfd = {conn->fd, POLLERR, 0};
            // poll() flags from the application's intent
            if(conn->want_read){
                pfd.events |= POLLIN;
            }
            if(conn->want_write){
                pfd.events |= POLLOUT;
            }

            poll_args.push_back(pfd);
        }

        // wait for readiness
        int rv = poll(poll_args.data(), (nfds_t)poll_args.size(), -1);
        if (rv < 0 && errno == EINTR){
            continue;
        }

        if (rv < 0 ){
            die("poll");
        }

        // handle listening socket
        if (poll_args[0].revents){
            if(Conn *conn = handle_accept(fd)){
                //put it into the map
                if(fd2conn.size() <= (size_t)conn->fd){
                    fd2conn.resize(conn->fd +1);
                }
                fd2conn[conn->fd] = conn;
            }
        }
        //handle connection sockets
        for (size_t i = 1; i < poll_args.size(); i++){
            uint32_t ready = poll_args[i].revents;
            if(ready == 0){
                continue;
            }

            Conn *conn = fd2conn[poll_args[i].fd];
            if (ready & POLLIN){
                assert(conn->want_read);
                handle_read(conn); // app logic
            }
            if(ready & POLLOUT){
                assert(conn->want_write);
                handle_write(conn); // app logic
            }

            // handle closing socket due to error or app logic
            if((ready & POLLERR) || conn->want_close){
                (void)close(conn->fd);
                fd2conn[conn->fd] = NULL;
                delete conn;
            }

        } // for each connection socket
    }  // the event loop
    return 0;
}