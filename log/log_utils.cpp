#include "log_utils.h"
#include <stdlib.h>

void die(const char *msg){
    int err = errno;
    fprintf(stderr, "[%d] %s\n", err, msg);
    abort();
}

void msg(const char *msg){
    fprintf(stderr, "%s\n", msg);
}

void msg_errno(const char *msg){
    fprintf(stderr, "[errno:%d] %s\n", errno, msg);
}