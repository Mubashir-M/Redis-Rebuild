#ifndef LOG_UTILS_H
#define LOG_UTILS_H

#include <stdio.h>
#include <errno.h>

void die(const char *msg);
void msg(const char *msg);
void msg_errno(const char *msg);

#endif