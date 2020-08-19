#include "service.h"

#include <stdlib.h>
#include <stdbool.h>
#include <errno.h>
#include <netdb.h>
#include <pthread.h>

#define MAX_BUF 2048

static bool str_to_port(const char *service, uint16_t *port)
{
    char *end = NULL;
    long l = strtol(service, &end, 10);

    if(errno == ERANGE || *end != '\0')
        return false;
    
    *port = htons((uint16_t)l);
    return true;
}

pthread_mutex_t getservbyname_lock = PTHREAD_MUTEX_INITIALIZER;

static bool query_service(const char *service, const char *proto, uint16_t *port)
{
#ifdef HAVE_GNU_GETSERVBYNAME_R
    {
        struct servent result_serven;
        struct servent *result;
        char buf[MAX_BUF];
        int ret = getservbyname_r(service, proto, &result_serven, buf, MAX_BUF, &result);
        if(ret != 0 || result == NULL)
            return false;
        
        *port = (uint16_t)result_serven.s_port;
        return true;
    }
#else
    {
        if (pthread_mutex_lock(&getservbyname_lock) != 0) {
            return false;
        }

        bool result = false;
        struct servent *res = getservbyname(service, proto);
        if(res) {
            *port = (uint16_t)res->s_port;
            result = true;
        }

        pthread_mutex_unlock(&getservbyname_lock);

        return result;
    }
#endif
    
    return false;
}

/*
 *
 * public functions
 *
 */
uint16_t portbyservice(const char *service, const char *proto)
{
    if(!service || !service[0])
        return htons(0);
    
    uint16_t port = 0;

    if(str_to_port(service, &port))
        return port;
    
    if(query_service(service, proto, &port))
        return port;

    return htons(0);
}
