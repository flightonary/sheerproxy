#include "connect.h"
#include "config.h"
#include "proxy.h"
#include "dns.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <netdb.h>

#define CONNECT_SUCCESS  0
#define CONNECT_ERROR   -1

#define PORT_BUF_SIZE 10

#define     SATOSIN(x)      ((struct sockaddr_in *) x)
#define     SATOSIN6(x)     ((struct sockaddr_in6 *) x)
#define     SOCKADDR(x)     (SATOSIN(x)->sin_addr)
#define     SOCKADDR6(x)    (SATOSIN6(x)->sin6_addr)
#define     SOCKFAMILY(x)   (SATOSIN(x)->sin_family)
#define     SOCKPORT(x)     (SATOSIN(x)->sin_port)
#define     SOCKPORT6(x)    (SATOSIN6(x)->sin6_port)

static connect_t original_connect;
static getaddrinfo_t original_getaddrinfo;

typedef struct proxy_info {
    struct sockaddr       *sa;
    socklen_t           salen;
    const char          *user;
    const char          *pass;
} proxy_info_t;

void free_proxyinfo(proxy_info_t *p)
{
    if(p->sa)
        free(p->sa);
    free(p);
}

bool is_stream(int sock) {
    int socktype = 0;
    socklen_t optlen = sizeof(int);
    getsockopt(sock, SOL_SOCKET, SO_TYPE, &socktype, &optlen);

    if(socktype != SOCK_STREAM)
        return false;
    
    return true;
}

int proxy_addrinfo(const char *node, const char *port, sa_family_t family, struct addrinfo **ret) {
    struct addrinfo hints = {0};
    hints.ai_family = family;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_V4MAPPED | AI_ADDRCONFIG | AI_NUMERICSERV;
    if(family != AF_UNSPEC)
        hints.ai_flags |= AI_NUMERICHOST;

    return original_getaddrinfo(node, port, &hints, ret);
}

proxy_info_t* select_proxy(sa_family_t family)
{
    // TODO: how to select proxy?

    const sp_config_t *cfg = get_config();
    
    proxy_info_t *proxy_info = NULL;
    struct http_proxy *hp = cfg->hp;
    
    for(; hp != NULL; hp = hp->next) {
        if(family == AF_INET && hp->addr_family == AF_INET6)
            continue;

        struct addrinfo *proxy_ai = NULL;
        int ret = proxy_addrinfo(hp->domain, hp->port, hp->addr_family, &proxy_ai);

        if(ret != 0) {
            freeaddrinfo(proxy_ai);
            continue;
        }
        
        // TODO: check proxy connectivity

        proxy_info = calloc(1, sizeof(proxy_info_t));
        proxy_info->sa = calloc(1, proxy_ai->ai_addrlen);
        memcpy(proxy_info->sa, proxy_ai->ai_addr, proxy_ai->ai_addrlen);
        proxy_info->salen =  proxy_ai->ai_addrlen;
        proxy_info->user = hp->user;
        proxy_info->pass = hp->pass;
        
        freeaddrinfo(proxy_ai);
        
        break;
    }
    
    return proxy_info;
}


/*
 *
 * public functions
 *
 */

void init_connect(connect_t connect, getaddrinfo_t getaddrinfo)
{
    original_connect = connect;
    original_getaddrinfo = getaddrinfo;
}

int sp_connect(int sock, const struct sockaddr *sa, socklen_t salen) {
    sa_family_t family = SOCKFAMILY(sa);
    char* domain = NULL;
    char  port[PORT_BUF_SIZE] = {0};
    
    if (is_stream(sock) == false)
        goto original_behavior;

    if (family == AF_INET) {
        domain = querybyaddr(&SOCKADDR(sa));
        snprintf(port, PORT_BUF_SIZE, "%d", ntohs(SOCKPORT(sa)));
    } else if (family == AF_INET6) {
        domain = querybyaddr6(&SOCKADDR6(sa));
        snprintf(port, PORT_BUF_SIZE, "%d", ntohs(SOCKPORT6(sa)));
    }
    if(!domain)
        goto original_behavior;

    proxy_info_t *proxy_info = select_proxy(family);
    if(!proxy_info){
        free(domain);
        errno = ECONNREFUSED;
        return CONNECT_ERROR;
    }
    
    int ret = proxy(sock, domain, port,
                    proxy_info->sa, proxy_info->salen,
                    proxy_info->user, proxy_info->pass);
    
    free(domain);
    free_proxyinfo(proxy_info);
    return ret;

original_behavior:
    return original_connect(sock, sa, salen);
}
