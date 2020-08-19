#include <netinet/in.h>
#include <sys/socket.h>

#ifndef dns_h
#define dns_h

struct sp_addr {
    struct  in_addr sin_addr;
    struct in6_addr sin6_addr;
};

struct sp_addr* querybyname(const char *);
char* querybyaddr(const struct in_addr *);
char* querybyaddr6(const struct in6_addr *);

#endif /* dns_h */
