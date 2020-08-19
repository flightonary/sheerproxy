#include "libsheerproxy.h"

#include <stdbool.h>
#include <netdb.h>

#ifndef config_h
#define config_h

#define MAX_USER_LEN 256
#define MAX_PASS_LEN 256

struct always_direct_v4 {
    struct in_addr           addr;
    struct in_addr           mask;
    struct always_direct_v4 *next;
};

struct always_direct_v6 {
    struct in6_addr          addr;
    struct in6_addr          mask;
    struct always_direct_v6 *next;
};

struct always_direct_domain {
    char                      *domain;
    struct always_direct_domain *next;
};

struct port_restriction {
    uint16_t port;
    struct port_restriction *next;
};

struct http_proxy {
    char               *domain;
    char                 *port;
    char                 *user;
    char                 *pass;
    sa_family_t    addr_family;
    struct in_addr     *inaddr;
    struct in6_addr   *in6addr;
    struct http_proxy    *next;
};

typedef struct sp_config {
    struct always_direct_v4         *ad_v4;
    struct always_direct_v6         *ad_v6;
    struct always_direct_domain *ad_domain;
    struct port_restriction            *pr;
    struct http_proxy                  *hp;
} sp_config_t;

const sp_config_t* get_config(void);
bool read_config(const char*);

#endif /* config_h */
