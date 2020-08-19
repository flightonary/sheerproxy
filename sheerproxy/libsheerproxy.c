#include "libsheerproxy.h"
#include "config.h"
#include "connect.h"
#include "getaddrinfo.h"
#include "proxy.h"

#include <stdlib.h>
#include <stdbool.h>
#include <dlfcn.h>
#include <pthread.h>

connect_t original_connect;
getaddrinfo_t original_getaddrinfo;
getnameinfo_t original_getnameinfo;

#ifndef SYSCONFDIR
#define SYSCONFDIR "/etc/sheerproxy"
#endif

#define SYSCONF SYSCONFDIR "/sheerproxy.conf"

/*
 * helper
 */
static bool load_original_functions(void) {
    original_connect = dlsym(RTLD_NEXT, "connect");
    original_getaddrinfo = dlsym(RTLD_NEXT, "getaddrinfo");
    original_getnameinfo = dlsym(RTLD_NEXT, "getnameinfo");
    
    if (!original_connect || !original_getaddrinfo || !original_getnameinfo) {
        return false;
    }
    
    return true;
}

static const char* get_config_path(void)
{
    const char* path = NULL;
    if((path = getenv("SHEERPROXY_CONF")) != NULL)
        return path;
    
    return SYSCONF;
}

/*
 * initilizer
 */
static bool passthrough_mode = true;

static void init_lib(void)
{
    if(!load_original_functions())
        return;

    if(getenv("NO_SHEERPROXY") != NULL)
        return;
    
    if(!read_config(get_config_path()))
        return;
    
    init_connect(original_connect, original_getaddrinfo);
    init_getaddrinfo(original_getaddrinfo);
    init_proxy(original_connect);
    
    // all initializations are success.
    passthrough_mode = false;
}

static pthread_once_t init_once = PTHREAD_ONCE_INIT;

__attribute__((constructor)) static void init_lib_once() {
    pthread_once(&init_once, init_lib);
}

/*
 *
 * Intercept functions
 *
 */
int connect(int sock, const struct sockaddr *sa, socklen_t salen) {
    if(passthrough_mode)
        return original_connect(sock, sa, salen);
    
    return sp_connect(sock, sa, salen);
}

int getaddrinfo(const char *node, const char *service,
                const struct addrinfo *hints, struct addrinfo **res) {
    if(passthrough_mode)
        return original_getaddrinfo(node, service, hints, res);
    
    return sp_getaddrinfo(node, service, hints, res);
}

int getnameinfo_bridge(const void *sa, socklen_t salen,
                       char *host, socklen_t hostlen,
                       char *serv, socklen_t servlen, int flags) {
    if(passthrough_mode)
        goto original_behavior;

original_behavior:
    return original_getnameinfo(sa, salen, host, hostlen, serv, servlen, flags);
}
