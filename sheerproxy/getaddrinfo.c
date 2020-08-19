#include "getaddrinfo.h"
#include "config.h"
#include "dns.h"
#include "addr.h"
#include "service.h"

#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <arpa/inet.h>

#define GETADDRINFO_SUCCESS 0

static getaddrinfo_t original_getaddrinfo;

/*
 * decision logics
 */
bool tcp_allowed(const struct addrinfo *hints)
{
    if(hints->ai_family != AF_UNSPEC &&
       hints->ai_family != AF_INET && hints->ai_family != AF_INET6)
        return false;
    
    if(hints->ai_protocol != IPPROTO_IP && hints->ai_protocol != IPPROTO_TCP)
        return false;
    
    if(hints->ai_socktype != 0 && hints->ai_socktype != SOCK_STREAM)
        return false;
    
    return true;
}

bool is_ignore_v4(const struct in_addr *inaddr)
{
    if(in_unspecified(inaddr) ||
       in_loopback(inaddr) ||
       in_linklocal(inaddr) ||
       in_multicast(inaddr))
        return true;
    
    return false;
}

bool is_ignore_v6(const struct in6_addr *in6addr)
{
    if(in6_unspecified(in6addr) ||
       in6_loopback(in6addr) ||
       in6_linklocal(in6addr) ||
       in6_multicast(in6addr))
        return true;
    
    return false;
}

bool is_proxy(sa_family_t family, struct inaddr_storage *is, const char *node)
{
    const sp_config_t *cfg = get_config();
    
    for(struct http_proxy *hp = cfg->hp; hp != NULL; hp = hp->next) {
        if(family == AF_INET && hp->addr_family == AF_INET) {
            if(equal_in_addr((struct in_addr*)is, hp->inaddr))
                return true;
        } else if(family == AF_INET6 && hp->addr_family == AF_INET6) {
            if(equal_in6_addr((struct in6_addr*)is, hp->in6addr))
                return true;
        } else if(family == AF_UNSPEC && hp->addr_family == AF_UNSPEC) {
            if(equal_domain(node, hp->domain))
                return true;
        }
    }

    return false;
}

bool is_always_direct_v4(const struct in_addr *inaddr)
{
    const sp_config_t *cfg = get_config();
    
    for(struct always_direct_v4 *ad = cfg->ad_v4; ad != NULL; ad = ad->next) {
        if(subnet_of_v4(inaddr, &ad->mask, &ad->addr))
            return true;
    }
    return false;
}

bool is_always_direct_v6(const struct in6_addr *in6addr)
{
    const sp_config_t *cfg = get_config();
    
    for(struct always_direct_v6 *ad = cfg->ad_v6; ad != NULL; ad = ad->next) {
        if(subnet_of_v6(in6addr, &ad->mask, &ad->addr))
            return true;
    }
    return false;
}

bool is_always_direct_domain(const char *node)
{
    const sp_config_t *cfg = get_config();
    
    for(struct always_direct_domain *ad = cfg->ad_domain; ad != NULL; ad = ad->next) {
        if(is_parent_domain(ad->domain, node))
            return true;
    }
    return false;
}

/*
 * service restriction
 */
bool is_restricted_service_to_proxy(const char *service)
{
    const sp_config_t *cfg = get_config();
    if(!cfg->pr)
        return true;

    uint16_t port = portbyservice(service, NULL);

    for(struct port_restriction *pr = cfg->pr; pr != NULL; pr = pr->next) {
        if(pr->port == port)
            return true;
    }

    return false;
}

/*
 * addrinfo
 */
int base_addrinfo(const char *node, const char *service,
                  const struct addrinfo *original_hints, struct addrinfo **ret) {
    struct addrinfo hints = {0};
    memcpy(&hints, original_hints, sizeof(struct addrinfo));
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags &= ~AI_CANONNAME;

    return original_getaddrinfo(node, service, &hints, ret);
}

bool addrinfo_from(struct sp_addr *spaddr, const char *node, const char *service,
                   const struct addrinfo *hints, struct addrinfo **res)
{
    const struct sp_config *cfg = get_config();
    
    struct http_proxy *hp = cfg->hp;

    if(base_addrinfo(hp->domain, service, hints, res) != 0)
        return false;
    
    // TODO: how to treat with base addrinfo in the case of ipv4/v6 dual stack?
    
    for(struct addrinfo *ai = *res; ai != NULL; ai = ai->ai_next){
        if(ai->ai_family == AF_INET6) {
            struct sockaddr_in6 *sa6 = (struct sockaddr_in6 *)ai->ai_addr;
            if(in6_v4mapped(&sa6->sin6_addr)) {
                struct in6_addr tmpin6 = {0};
                in6_to_v4mapped(&spaddr->sin_addr, &tmpin6);
                memcpy(&sa6->sin6_addr, &tmpin6, sizeof(struct in6_addr));
            } else {
                memcpy(&sa6->sin6_addr, &spaddr->sin6_addr, sizeof(struct in6_addr));
            }
        } else if(ai->ai_family == AF_INET) {
            struct sockaddr_in *sa = (struct sockaddr_in *)ai->ai_addr;
            memcpy(&sa->sin_addr, &spaddr->sin_addr, sizeof(struct in_addr));
        }
    }
    
    if(hints->ai_flags & AI_CANONNAME) {
        (*res)->ai_canonname = strndup(node, MAX_DOMAIN_LEN);
    }
    
    return true;
}


/*
 *
 * public functions
 *
 */

void init_getaddrinfo(getaddrinfo_t getaddrinfo)
{
    original_getaddrinfo = getaddrinfo;
}

int sp_getaddrinfo(const char *node, const char *service,
                   const struct addrinfo *hints, struct addrinfo **res) {
    if(!node || !node[0])
        goto original_behavior;
    
    if(hints && !tcp_allowed(hints))
        goto original_behavior;
    
    char domain[MAX_DOMAIN_LEN + 1] = {0};
    avoid_v6_scope(domain, node, MAX_DOMAIN_LEN);

    struct inaddr_storage addr_storage;
    sa_family_t family = get_inaddr(domain, &addr_storage);
    
    // Default direct check
    if((family == AF_INET  && is_ignore_v4((struct in_addr*)&addr_storage)) ||
       (family == AF_INET6 && is_ignore_v6((struct in6_addr*)&addr_storage))) {
        goto original_behavior;
    }
    
    // Proxy check
    if(is_proxy(family, &addr_storage, domain))
        goto original_behavior;
    
    // TODO: /etc/hostsに記載のあるホストは direct接続
    // TODO:  docker conteinerなど、dnsが特殊なケース向けに、domainがurlではないケースを考える

    // Passive direct check
    if((family == AF_INET   && is_always_direct_v4((struct in_addr*)&addr_storage))  ||
       (family == AF_INET6  && is_always_direct_v6((struct in6_addr*)&addr_storage)) ||
       (family == AF_UNSPEC && is_always_direct_domain(domain))) {
        goto original_behavior;
    }

    if(!is_restricted_service_to_proxy(service))
        goto original_behavior;
    
    struct sp_addr *spaddr = querybyname(domain);
    if(spaddr == NULL)
        goto original_behavior;
    
    if(!addrinfo_from(spaddr, node, service, hints, res)){
        free(spaddr);
        goto original_behavior;
    }
    
    free(spaddr);
    return GETADDRINFO_SUCCESS;

original_behavior:
    return original_getaddrinfo(node, service, hints, res);
}
