#ifndef testutils_h
#define testutils_h

#include <stdio.h>
#include <netdb.h>

#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>

#define TEST(x) do { test(x); } while(0);
#define TEST_DONE do { test_done(); } while(0);
#define TEST_LOG "  [test log] "

void test(const char *desc)
{
    static int test_num = 1;
    printf("TEST:%d %s\n", test_num++, desc);
}

void test_done(void)
{
    printf("** TEST SUCCESS **\n");
}

void print_ipv4(struct in_addr ipv4)
{
    printf(TEST_LOG "ipv4 = %s\n", inet_ntoa(ipv4));
}

void print_port(uint16_t port)
{
    printf(TEST_LOG "port = %d\n", ntohs(port));
}

void print_addrinfo(struct addrinfo *ai)
{
    if(!ai)
        return;
    
    while(1) {
        int family = ai->ai_family;
        
        printf(TEST_LOG);
        printf("flags: %d, family: %d, socktype: %d, protocol: %d, canonname: %s, ",
               ai->ai_flags, ai->ai_family, ai->ai_socktype, ai->ai_protocol, ai->ai_canonname);

        struct sockaddr *sa = ai->ai_addr;
        char ip_str[128];
        int port = 0;
        
        if(family == AF_INET) {
            struct sockaddr_in *sain = (struct sockaddr_in *)sa;
            struct in_addr *inaddr = &(sain->sin_addr);
            inet_ntop(family, inaddr, ip_str, sizeof(ip_str));
            port = ntohs(sain->sin_port);
        } else if(family == AF_INET6) {
            struct sockaddr_in6 *sain6 = (struct sockaddr_in6 *)sa;
            struct in6_addr *inaddr6 = &(sain6->sin6_addr);
            inet_ntop(family, inaddr6, ip_str, sizeof(ip_str));
            port = ntohs(sain6->sin6_port);
        }
        printf("addr: %s, port: %d\n", ip_str, port);
        
        if(!ai->ai_next)
            break;
        
        ai = ai->ai_next;
    }
}

#endif /* testutils_h */
