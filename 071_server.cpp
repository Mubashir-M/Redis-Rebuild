#include "buffer_operations.h"
#include "connection_handlers.h"
#include "data_store.h"
#include "log_utils.h"
#include "protocol_serialization.h"
#include "server_common.h"
#include "server_config.h"
#include "socket_utils.h"
#include "hashmap.h"
#include "DList.h"
#include "utils/timer.h"

#include <sys/socket.h>
#include <poll.h>
#include <assert.h>
#include <unistd.h>
#include <netinet/in.h>


static int32_t next_timer_ms(){
    if(dlist_empty(&g_data.idle_list)){
        return -1; // no timers, no timeouts
    }

    uint64_t now_ms = get_monotonic_msec();
    Conn *conn = container_of(g_data.idle_list.next, Conn, idle_node);
    uint64_t next_ms = conn->last_active_ms + k_idle_timeout_ms;
    if(next_ms <= now_ms){
        return 0; //missed ?
    }
    return (int32_t)(next_ms - now_ms);

};

static void conn_destroy(Conn *conn){
    (void)close(conn->fd);
    g_data.fd2conn[conn->fd] = NULL;
    dlist_detach(&conn->idle_node);
    delete conn;
};

static void process_timers(){
    uint64_t now_ms = get_monotonic_msec();
    while(!dlist_empty(&g_data.idle_list)){
        Conn *conn = container_of(g_data.idle_list.next, Conn, idle_node);
        uint64_t next_ms = conn->last_active_ms + k_idle_timeout_ms;
        if(next_ms >= now_ms){
            break; // not expired
        }

        fprintf(stderr, "removing idle connection: %d\n", conn->fd);
        conn_destroy(conn);
    }
};

int main(){

    // initialization
    dlist_init(&g_data.idle_list);

    // the listenin socket
    int fd = socket(AF_INET, SOCK_STREAM, 0);

    if (fd == -1 ){
        die("socket()");
    }

    int val = 1;

    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val)) == -1 ){
        die("setsockopt()");
    }

    // bind
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
    
    // the event loop
    std::vector<struct pollfd> poll_args;

    while(true){
        //prepare the argument for the poll()
        poll_args.clear();
        // put the listening socket in the first position
        struct pollfd pfd = {fd, POLLIN, 0}; //POLLIN or POLLIN
        poll_args.push_back(pfd);
        // the rest are connection sockets
        for (Conn *conn : g_data.fd2conn){
            if(!conn){
                continue;
            }

            // always poll for error
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
        int32_t timeout_ms = next_timer_ms();
        int rv = poll(poll_args.data(), (nfds_t)poll_args.size(), timeout_ms);
        if (rv < 0 && errno == EINTR){
            continue; // not an error
        }

        if (rv < 0 ){
            die("poll");
        }

        // handle listening socket
        if (poll_args[0].revents){
           handle_accept(fd);
        }

        //handle connection sockets
        for (size_t i = 1; i < poll_args.size(); ++i){
            uint32_t ready = poll_args[i].revents;
            if(ready == 0){
                continue;
            }

            Conn *conn = g_data.fd2conn[poll_args[i].fd];

            // update the idle timer by moving the conn to the end of the list

            conn->last_active_ms = get_monotonic_msec();
            dlist_detach(&conn->idle_node);
            dlist_insert_before(&g_data.idle_list, &conn->idle_node);

            // handle IO
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
                conn_destroy(conn);
            }

        } // for each connection socket

        //handle timers
        process_timers();

    }  // the event loop
    return 0;
}