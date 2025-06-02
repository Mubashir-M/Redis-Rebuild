#include <poll.h>
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <iostream>
#include <vector>
#include <netinet/in.h>
#include <unistd.h>

struct Conn {
    int fd = -1;

    bool want_read = false;
    bool want_write = false;
    bool want_close = false;

    std::vector<uint8_t> incoming;
    std::vector<uint8_t> outgoing;
};

struct pollfd {
    int fd;
    short events;
    short revents;
};

static void die(const char *msg){
    int err = errno;
    fprintf(stderr, "[%d] %s\n", err, msg);
    abort();
}

static void msg(const char *msg){
    fprintf(stderr, "%s\n",msg);
};

const size_t k_ma_msg = 4096;

static void fd_set_nb(int fd){
    fcntl(fd, F_SETFL, fcntl(fd, F_GETFL,0 | O_NONBLOCK));
};

static void buf_append(std::vector<uint8_t> &buf, const uint8_t *data, size_t len){
    buf.insert(buf.end(), data, data + len);
};

static void buf_consume(std::vector<uint8_t> &buf, size_t n){
    buf.erase(buf.begin(), buf.begin() + n);
};

static Conn* handle_accept(int fd){

};

static void handle_read(Conn *conn){

}

static void handle_write(Conn *conn)){
    
}

int main(){
    int fd = socket(AF_INET, SOCK_STREAM, 0);

    if(fd < 0){
        die("socket()");
    }

    int val = 1;

    if (setsockopt(fd, SOL_SOCKET ,SO_REUSEADDR, &val, sizeof(val)) == -1){
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

    rv = listen(fd, SOMAXCONN);

    if (rv){
        die("listen()");
    }

    std::vector<Conn*> fd2conn;
    std::vector<struct pollfd> poll_args;

    while(true){
        poll_args.clear();

        struct pollfd pfd = {fd, POLL_IN, 0};
        poll_args.push_back(pfd);

        for(Conn *conn : fd2conn){
            if(!conn){
                continue;
            }

            struct pollfd pfd = {conn->fd, POLL_ERR, 0};
            if(conn->want_read){
                pfd.events |= POLL_IN;
            }

            if(conn->want_write){
                pfd.events |= POLL_OUT;
            }

            poll_args.push_back(pfd);
        }

        int rv = poll(poll_args.data(), (nfds_t)poll_args.size(), -1);
        if (rv < 0 && errno == EINTR){
            continue;
        }

        if (rv < 0 ){
            die("poll");
        }

        if(poll_args[0].revents){
            if(Conn *conn = handle_accept(fd)){
                if(fd2conn.size() < conn->fd){
                    fd2conn.resize(conn->fd +1);
                }
                fd2conn[conn->fd] = conn;
            }
        }


        for(size_t i = 0; i < poll_args.size(); i++){
            uint32_t ready = poll_args[i].revents;
            Conn *conn = fd2conn[poll_args[i].fd];

            if(ready & POLL_IN){
                handle_read(conn);
            }

            if(ready & POLL_OUT){
                handle_write(conn);
            }

            if((ready & POLL_ERR) || conn->want_close){
                (void)close(conn->fd);
                fd2conn[conn->fd] = NULL;
                delete conn;
            }
        }


    }
    return 0;
}