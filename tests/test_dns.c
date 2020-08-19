#include "testutils.h"
#include "dns.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>


int main(int argc, char** argv) {
    
    /*
        querybyname tests
     */
    TEST("querybyname: null check")
    struct sp_addr *spaddr_null = querybyname(NULL);
    assert(!spaddr_null);
    
    TEST("querybyname: query example.com")
    struct sp_addr *spaddr_example_1 = querybyname("example.com");
    assert(spaddr_example_1);
    assert( !strcmp(inet_ntoa(spaddr_example_1->sin_addr), "255.255.255.254") ); // "255.255.255.254" is initial offset
    
    TEST("querybyname: query apple.com")
    const char* apple = "apple.com";
    struct sp_addr *spaddr_apple = querybyname(apple);
    assert(spaddr_apple);

    TEST("querybyname: query example.com again")
    struct sp_addr *spaddr_example_2 = querybyname("example.com");
    assert( !memcmp(spaddr_example_1, spaddr_example_2, sizeof(struct sp_addr)) );
    
    TEST("querybyname: query google.com")
    const char* google = "google.com";
    struct sp_addr *spaddr_google = querybyname(google);
    assert( memcmp(spaddr_apple, spaddr_google, sizeof(struct sp_addr)) );
    
    /*
        querybyaddr tests
     */
    TEST("querybyaddr: query addr(ipv4) assigned to google.com above")
    const char* query_google = NULL;
    query_google = querybyaddr(&(spaddr_google->sin_addr));
    assert(query_google);
    assert(!strcmp(query_google, google));

    TEST("querybyaddr: query addr(ipv6) assigned to google.com above")
    query_google = querybyaddr6(&(spaddr_google->sin6_addr));
    assert(query_google);
    assert(!strcmp(query_google, google));

    TEST("querybyaddr: query non-exist addr(ipv4)")
    const char* nonexist_node = NULL;
    uint32_t nonexist_ipv4 = htonl(0x00000fb1);
    nonexist_node = querybyaddr((struct in_addr*)&nonexist_ipv4);
    assert(!nonexist_node);
    
    TEST_DONE
    return 0;
}
