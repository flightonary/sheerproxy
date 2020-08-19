#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>

#include "testutils.h"
#include "proxy.h"

#define CONNECT_ERROR -1

int main(int argc, char** argv) {
    /*
        posix tests
     */
    {
        TEST("posix behavior tests")
        int so1 = socket(AF_INET, SOCK_STREAM, 0);
        
        int so1_keepalive = 1;
        setsockopt(so1, SOL_SOCKET, SO_KEEPALIVE, &so1_keepalive, sizeof(int));
        
        int so2 = socket(AF_INET, SOCK_STREAM, 0);
        int so2_keepalive = 0;
        setsockopt(so2, SOL_SOCKET, SO_KEEPALIVE, &so2_keepalive, sizeof(int));

        dup2(so2, so1);
        so1_keepalive = -1;
        socklen_t optlen = sizeof(so1_keepalive);
        getsockopt(so1, SOL_SOCKET, SO_KEEPALIVE, &so1_keepalive, &optlen);
        assert(so1_keepalive == 0);
        //   It means that so1's sockopts will be reset if use dup2.
        //   dup2 should not be used to preserve the original socket options.
    }
    
    /*
        preparation
     */
    init_proxy(connect);

    /*
        porxy tests
     */
    {
        TEST("proxy: args check test")
        int ret1 = proxy(0, NULL, 0, NULL, 0, NULL, NULL);
        assert(errno != 0);
        assert(ret1 == CONNECT_ERROR);
    }

    {
        TEST("proxy: do proxy successfully")
        struct addrinfo hints, *proxy_ai;
        memset(&hints, 0, sizeof(hints));
        hints.ai_socktype = SOCK_STREAM;
        int ret_ai = getaddrinfo("squid", "3128", &hints, &proxy_ai);
        assert(ret_ai == 0);
        print_addrinfo(proxy_ai);
        
        int sock = socket(AF_INET, SOCK_STREAM, 0);
        assert(sock != -1);

        int ret = proxy(sock, "example.com", "80", proxy_ai->ai_addr, proxy_ai->ai_addrlen, NULL, NULL);
        assert(errno == 0);
        assert(ret == 0);
    }
    
    {
        TEST("proxy: do proxy with basic auth successfully")
        struct addrinfo hints, *proxy_ai = NULL;
        memset(&hints, 0, sizeof(hints));
        hints.ai_socktype = SOCK_STREAM;
        getaddrinfo("squid-basic", "3129", &hints, &proxy_ai);
        print_addrinfo(proxy_ai);

        int sock = socket(AF_INET, SOCK_STREAM, 0);
        assert(sock != -1);

        int ret = proxy(sock, "example.com", "80", proxy_ai->ai_addr, proxy_ai->ai_addrlen, "user01", "user01");
        assert(errno == 0);
        assert(ret == 0);
        
        TEST("proxy: basic authentication failed test")
        ret = proxy(sock, "example.com", "80", proxy_ai->ai_addr, proxy_ai->ai_addrlen, "user01", "wrongpass");
        assert(errno != 0);
        assert(ret != 0);
    }

    TEST_DONE
    return 0;
}


// memo:
//struct addrinfo {
//    int              ai_flags;
//    int              ai_family;
//    int              ai_socktype;
//    int              ai_protocol;
//    socklen_t        ai_addrlen;
//    struct sockaddr *ai_addr;
//    char            *ai_canonname;
//    struct addrinfo *ai_next;
//};
