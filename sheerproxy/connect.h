#include "libsheerproxy.h"

#include <netdb.h>

#ifndef connect_h
#define connect_h

void init_connect(connect_t, getaddrinfo_t);
int sp_connect(int, const struct sockaddr*, socklen_t);

#endif /* connect_h */
