#include "libsheerproxy.h"

#include <netinet/in.h>

#ifndef proxy_h
#define proxy_h

void init_proxy(connect_t);
int proxy(int, const char *, const char *, const struct sockaddr *, socklen_t, const char *, const char *);

#endif /* proxy_h */
