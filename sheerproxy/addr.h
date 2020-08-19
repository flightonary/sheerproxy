#ifndef addr_h
#define addr_h

#include <stdbool.h>
#include <netdb.h>

struct inaddr_storage {
    char   data[16];
};

void avoid_dns_root(char *, const char *, size_t);
void avoid_v6_scope(char *, const char *, size_t);

bool equal_in_addr(const struct in_addr *, const struct in_addr *);
bool equal_in6_addr(const struct in6_addr *, const struct in6_addr *);
bool equal_domain(const char *, const char *);
bool subnet_of_v4(const struct in_addr *, const struct in_addr *, const struct in_addr *);
bool subnet_of_v6(const struct in6_addr *, const struct in6_addr *, const struct in6_addr *);
bool is_parent_domain(const char *, const char *);

bool in_unspecified(const struct in_addr *);
bool in_loopback(const struct in_addr *);
bool in_linklocal(const struct in_addr *);
bool in_multicast(const struct in_addr *);

bool in6_unspecified(const struct in6_addr *);
bool in6_loopback(const struct in6_addr *);
bool in6_linklocal(const struct in6_addr *);
bool in6_multicast(const struct in6_addr *);
bool in6_v4mapped(const struct in6_addr *);

sa_family_t get_inaddr(const char *, struct inaddr_storage *);
void in6_to_v4mapped(const struct in_addr *, struct in6_addr *);

#endif /* addr_h */
