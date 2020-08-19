#include <netdb.h>

#ifndef libsheerproxy_h
#define libsheerproxy_h

#define MAX_DOMAIN_LEN 255

typedef int     (*connect_t)      (int, const struct sockaddr *, socklen_t);
typedef int     (*getaddrinfo_t)  (const char *, const char *,
                                   const struct addrinfo *, struct addrinfo **);
//typedef int     (*freeaddrinfo_t) (struct addrinfo *);
typedef int     (*getnameinfo_t)  (const struct sockaddr *, socklen_t, char *,
                                   socklen_t, char *, socklen_t, int);

#endif /* libsheerproxy_h */
