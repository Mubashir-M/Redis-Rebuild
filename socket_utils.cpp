#include "socket_utils.h"

void fd_set_nb(int fd){
    fcntl(fd, F_SETFL, fcntl(fd, F_GETFL, 0 | O_NONBLOCK));
}