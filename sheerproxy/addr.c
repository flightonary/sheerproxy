#include "libsheerproxy.h"
#include "addr.h"

#include <string.h>
#include <arpa/inet.h>

static const struct in6_addr IN6ADDR_V4MAPPED_INIT_SP = \
                {{{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, \
                    0x00, 0x00, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00 }}};

/*
 *
 * public functions
 *
 */

/*
 * string address utilities
 */
void avoid_dns_root(char *dst, const char *src, size_t len)
{
    strncpy(dst, src, len);
    if(dst[strlen(dst) - 1] == '.')
        dst[strlen(dst) - 1] = '\0';
}

void avoid_v6_scope(char *dst, const char *src, size_t len)
{
    for(int i = 0; i < len; i++) {
        if(src[i] == '\0' || src[i] == '%') {
            dst[i] = '\0';
            break;
        }
        dst[i] = src[i];
    }
}

/*
 * address compare
 */
bool equal_in_addr(const struct in_addr *inaddr1, const struct in_addr *inaddr2)
{
    return (memcmp(inaddr1, inaddr2, sizeof(struct in_addr)) == 0);
}

bool equal_in6_addr(const struct in6_addr *inaddr1, const struct in6_addr *inaddr2)
{
    return (memcmp(inaddr1, inaddr2, sizeof(struct in6_addr)) == 0);
}

bool equal_domain(const char *domain1, const char *domain2)
{
    char tmpdomain1[MAX_DOMAIN_LEN + 1] = {0};
    char tmpdomain2[MAX_DOMAIN_LEN + 1] = {0};
    
    avoid_dns_root(tmpdomain1, domain1, MAX_DOMAIN_LEN);
    avoid_dns_root(tmpdomain2, domain2, MAX_DOMAIN_LEN);
    
    return (strcmp(tmpdomain1, tmpdomain2) == 0);
}

bool subnet_of_v4(const struct in_addr *addr, const struct in_addr *mask, const struct in_addr *network)
{
    return (*(uint32_t *)addr & *(uint32_t *)mask) == *(uint32_t *)network;
}

bool subnet_of_v6(const struct in6_addr *addr, const struct in6_addr *mask, const struct in6_addr *network)
{
    char masked_addr[16];
    for(int i = 0; i < 16; i++)
        masked_addr[i] = addr->s6_addr[i] & mask->s6_addr[i];
    
    return memcmp(masked_addr, network, 16);
}

bool is_parent_domain(const char *parent, const char *child)
{
    char tmpparent[MAX_DOMAIN_LEN + 1] = {0};
    char tmpchild[MAX_DOMAIN_LEN + 1] = {0};
    
    avoid_dns_root(tmpparent, parent, MAX_DOMAIN_LEN);
    avoid_dns_root(tmpchild, child, MAX_DOMAIN_LEN);
    
    size_t parentlen = strlen(tmpparent);
    size_t childlen = strlen(tmpchild);
    if (childlen > parentlen)
        return 0;
    return (strncmp(tmpparent + parentlen - childlen, tmpchild, childlen) == 0);
}

/*
 * classfy address
 */
bool in_unspecified(const struct in_addr *inaddr)
{
    // 0.0.0.0, 240.0.0.0 - 255.255.255.255
    uint32_t addr = ntohl(inaddr->s_addr);
    return (addr == INADDR_ANY || ((addr & 0xf0000000) == 0xf0000000));
}

bool in_loopback(const struct in_addr *inaddr)
{
    // 127.0.0.0
    uint32_t addr = ntohl(inaddr->s_addr);
    return ((addr & 0xff000000) == 0x7f000000);
}

bool in_linklocal(const struct in_addr *inaddr)
{
    // 169.254.0.0
    uint32_t addr = ntohl(inaddr->s_addr);
    return ((addr & 0xffff0000) == 0xA9FE0000);;
}

bool in_multicast(const struct in_addr *inaddr)
{
    // 224.0.0.0 - 239.255.255.255
    uint32_t addr = ntohl(inaddr->s_addr);
    return ((addr & 0xf0000000) == 0xe0000000);
}

bool in6_unspecified(const struct in6_addr *in6addr)
{
    // ::, 20:01:0d:b8::
    uint32_t *prefix = (uint32_t*)(&in6addr->s6_addr[0]);
    return IN6_IS_ADDR_UNSPECIFIED(in6addr) || *prefix == htonl(0x20010db8);
}

bool in6_loopback(const struct in6_addr *in6addr)
{
    // ::1
    return IN6_IS_ADDR_LOOPBACK(in6addr);
}

bool in6_linklocal(const struct in6_addr *in6addr)
{
    // fe80::
    return IN6_IS_ADDR_LINKLOCAL(in6addr);
}

bool in6_multicast(const struct in6_addr *in6addr)
{
    // ff::
    return IN6_IS_ADDR_MULTICAST(in6addr);
}

bool in6_v4mapped(const struct in6_addr *in6addr)
{
    return IN6_IS_ADDR_V4MAPPED(in6addr);
}

/*
 * addr utilities
 */
sa_family_t get_inaddr(const char *node, struct inaddr_storage *addr)
{
    if (inet_pton(AF_INET, node, addr)) {
        return AF_INET;
    } else if (inet_pton(AF_INET6, node, addr)) {
        return AF_INET6;
    }

    return AF_UNSPEC;
}

void in6_to_v4mapped(const struct in_addr *inaddr, struct in6_addr *in6addr)
{
    struct in6_addr tmpin6 = IN6ADDR_V4MAPPED_INIT_SP;
    memcpy(&tmpin6.s6_addr[12], inaddr, sizeof(struct in_addr));
    memcpy(in6addr, &tmpin6, sizeof(struct in6_addr));
}
