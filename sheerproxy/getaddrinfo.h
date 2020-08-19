#include "libsheerproxy.h"

#include <netdb.h>

#ifndef getaddrinfo_h
#define getaddrinfo_h

void init_getaddrinfo(getaddrinfo_t);

int sp_getaddrinfo(const char *node, const char *service,
                   const struct addrinfo *hints, struct addrinfo **res);

#endif /* getaddrinfo_h */
