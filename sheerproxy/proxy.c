#include "proxy.h"
#include "http.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <errno.h>
#include <fcntl.h>

#define CONNECT_SUCCESS  0
#define CONNECT_ERROR   -1

#define BUFF_SIZE (2*1024)

#define ENOERR 0

static int handle_error(int err)
{
    errno = err;

    if(err) {
        return CONNECT_ERROR;
    }
    
    errno = ENOERR;
    return CONNECT_SUCCESS;
}

static bool is_end_of_response(const char *str)
{
    static const char *post = "\r\n\r\n";

    return strncmp(post, str, strlen(post)) == 0;
}

static bool is_http_200(const char *str)
{
    // Http response should be "HTTP/1.1 200 Connection established\r\n\r\n"
    static const char *http_200_ok = "HTTP/1.1 200";

    return strncmp(http_200_ok, str, strlen(http_200_ok)) == 0;
}

static bool wait_for_200_response(int sock)
{
    char response[BUFF_SIZE] = {0};
    int resplen = 0;

    while(resplen < BUFF_SIZE) {
        int recvlen = (int)recv(sock, (char *)(response + resplen), BUFF_SIZE - resplen, 0);
        if(recvlen <= 0){
            return false;
        }
        
        for(int last = resplen + recvlen; resplen < last; resplen++) {
            if(resplen > 4 && is_end_of_response((char *)(response + resplen - 3)))
                goto endloop;
        }
    }
    
    return false;
    
endloop:
    if(!is_http_200(response))
       return false;
    
    return true;
}

static int intercept_http_connect(int sock, const char *node, const char *port, const char* user, const char* pass)
{
    char req[BUFF_SIZE] = {0};
    int reqlen = http_connect_request(node, port, user, pass, req, BUFF_SIZE);
    
    if(reqlen < 0 || reqlen != send(sock, req, reqlen, 0) || !wait_for_200_response(sock))
        return ECONNREFUSED;
    
    return ENOERR;
}

/*
 *
 * public functions
 *
 */
static connect_t original_connect = NULL;

void init_proxy(connect_t connect)
{
    original_connect = connect;
}

int proxy(int sock, const char *node, const char *port,
          const struct sockaddr *proxy_sa, socklen_t proxy_salen,
          const char* user, const char* pass)
{
    if (!node || !proxy_sa) {
        return handle_error(EINVAL);
    }
    
    int flags = fcntl(sock, F_GETFL, 0);
    int nonblocking_flag = flags & O_NONBLOCK;
    if(nonblocking_flag)
        fcntl(sock, F_SETFL, !O_NONBLOCK);
    
    original_connect(sock, proxy_sa, proxy_salen);
    int conn_err = errno;
    if(conn_err) {
        return handle_error(conn_err);
    }
    
    int int_err = intercept_http_connect(sock, node, port, user, pass);
    if(int_err) {
        return handle_error(int_err);
    }
    
    if(nonblocking_flag)
        fcntl(sock, F_SETFL, O_NONBLOCK);

    return handle_error(ENOERR);
}
