#include "testutils.h"
#include "service.h"

#include <stdio.h>
#include <assert.h>

int main(int argc, char** argv)
{
    /*
        portbyservice tests
     */
    {
        TEST("get port by numrical string(80)")
        uint16_t port = portbyservice("80", NULL);
        print_port(port);
        assert(port == htons(80));
    }
    
    {
        TEST("get port by numrical string with protcol")
        uint16_t port = portbyservice("443", "tcp");
        print_port(port);
        assert(port == htons(443));
    }
    
    {
        TEST("get port by service string(http)")
        uint16_t port = portbyservice("http", NULL);
        print_port(port);
        assert(port == htons(80));
    }
    
    {
        TEST("get port by service string(https)")
        uint16_t port = portbyservice("https", NULL);
        print_port(port);
        assert(port == htons(443));
    }
    
    {
        TEST("get port by service string(http) with protcol")
        uint16_t port = portbyservice("http", "tcp");
        print_port(port);
        assert(port == htons(80));
    }
    
    TEST_DONE
    return 0;
}
