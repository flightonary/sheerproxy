#include "http.h"
#include "ap_base64.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BUFF_SIZE (1024)
#define ERR_N (-1)

/*
 * http request field builder
 */
static int connect_request_line(const char *host, const char *port, char *line, int linelen)
{
    if(!host || !port || !line)
        return ERR_N;

    return snprintf(line, linelen, "CONNECT %s:%s HTTP/1.0\r\n", host, port);
}

static int host_header(const char *host, const char *port, char *field, int fieldlen)
{
    if(!host || !port || !field)
            return ERR_N;

    return snprintf(field, fieldlen, "Host: %s:%s\r\n", host, port);
}

static int basic_auth_header(const char* user, const char* pass, char *field, int fieldlen)
{
    if(!user || !pass || !field)
        return ERR_N;

    char raw_cred[BUFF_SIZE] = {0};
    int raw_len = snprintf(raw_cred, sizeof(raw_cred), "%s:%s", user, pass);

    char encoded_cred[BUFF_SIZE] = {0};
    int encoded_len = ap_base64encode_len(raw_len);
    ap_base64encode(encoded_cred, raw_cred, encoded_len);
    
    return snprintf(field, fieldlen, "Proxy-Authorization: Basic %s\r\n", encoded_cred);
}

/*
 *
 * public functions
 *
 */
int http_connect_request(const char *host, const char *port, const char* user, const char* pass, char *request, int requestlen)
{
    char line[BUFF_SIZE] = {0};
    connect_request_line(host, port, line, BUFF_SIZE);
    
    char host_field[BUFF_SIZE] = {0};
    host_header(host, port, host_field, BUFF_SIZE);
    
    char auth_field[BUFF_SIZE] = {0};
    basic_auth_header(user, pass, auth_field, BUFF_SIZE);
    
    return snprintf(request, requestlen, "%s%s%s\r\n", line, host_field, auth_field);
}
