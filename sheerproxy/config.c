#include "config.h"
#include "addr.h"

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <stdbool.h>

#include <arpa/inet.h>

#define BUF_SIZE  1024
#define MAX_WORDS   10

#define IPV4  4
#define IPV6  6
#define NOIP  0

int get_words(char *line, char **words, int wordslen)
{
    int c = 0, word_count = 0;
    
    do {
        switch(line[c]) {
        case ' ':
        case '\t':
        case '\n':
        case '\r':
            line[c] = '\0';
            break;
        default:
            if(word_count < wordslen){
                if(c == 0 || line[c-1] == '\0') {
                    words[word_count++] = &line[c];
                }
            }
        }
    } while(line[++c] != '\0');
    
    return word_count;
}

bool stol(char *str, long *l)
{
    char *end = NULL;
    *l = strtol(str, &end, 10);
    if(errno == ERANGE || *end != '\0')
        return false;
    
    return true;
}

bool get_mask_v4(char *mask, struct in_addr* inaddr)
{
    long lmask;
    if(!stol(mask, &lmask))
        return false;
    
    if(lmask < 0 || 32 < lmask)
        return false;

    uint32_t bitmask = 0;
    for(int i = 0; i < lmask; i++) {
        bitmask = (bitmask >> i) & 0x80000000;
    }
    
    inaddr->s_addr = htonl(bitmask);
    return true;
}

bool get_mask_v6(char *mask, struct in6_addr* in6addr)
{
    long lmask;
    if(!stol(mask, &lmask))
        return false;
    
    if(lmask < 0 || 128 < lmask)
        return false;

    uint32_t ipv6[4] = {0};

    long fill = lmask / 32;
    memset(ipv6, 0xff, fill);
    
    if(fill != 4){
        int mod = lmask % 32;
        for(int i = 0; i < mod; i++) {
            ipv6[fill] = (ipv6[fill] >> i) & 0x80000000;
        }
    }
    
    memcpy(in6addr, ipv6, sizeof(struct in6_addr));
    return in6addr;
}

bool empty(char **words, int wordslen)
{
    if(words[0] == NULL || wordslen == 0)
        return true;
    
    return false;
}

bool comment(char **words, int wordslen)
{
    if(wordslen > 0 && words[0][0] == '#')
        return true;

    return false;
}

bool always_direct(char **words, int wordslen, struct sp_config* config)
{
    if(strcmp(words[0], "always_direct"))
        return false;

    if(wordslen < 2)
        return false;
    
    struct inaddr_storage addr_storage;
    sa_family_t family = get_inaddr(words[1], &addr_storage);

    if (family == AF_INET) {
        if(wordslen > 3)
            return false;

        char *mask = (wordslen == 3 ? words[2] : "32");
        struct in_addr mask_addr;
        if(!get_mask_v4(mask, &mask_addr))
            return false;
        
        struct always_direct_v4* adv4 = calloc(1, sizeof(struct always_direct_v4));
        memcpy(&adv4->addr, &addr_storage, sizeof(struct in_addr));
        adv4->mask = mask_addr;
        
        adv4->next = config->ad_v4;
        config->ad_v4 = adv4;

    } else if (family == AF_INET6) {
        if(wordslen > 3)
            return false;

        char *mask = (wordslen == 3 ? words[2] : "128");
        struct in6_addr mask_addr6;
        if(!get_mask_v6(mask, &mask_addr6))
            return false;
                
        struct always_direct_v6* adv6 = calloc(1, sizeof(struct always_direct_v6));
        memcpy(&adv6->addr, &addr_storage, sizeof(struct in6_addr));
        adv6->mask = mask_addr6;
        
        adv6->next = config->ad_v6;
        config->ad_v6 = adv6;

    } else {
        if(wordslen > 2)
            return false;

        struct always_direct_domain* addom = calloc(1, sizeof(struct always_direct_domain));
        addom->domain = strndup(words[1], MAX_DOMAIN_LEN);
        
        addom->next = config->ad_domain;
        config->ad_domain = addom;
    }
    
    return true;
}

bool port_restriction(char **words, int wordslen, struct sp_config* config)
{
    if(strcmp(words[0], "port_restriction"))
        return false;

    if(wordslen < 2)
        return false;
    
    struct port_restriction* root_pr = NULL;
    
    for(int i = 1; i < wordslen; i++) {
        long port;
        if(!stol(words[i], &port) || port < 0 || 65535 < port)
            return false;
        
        struct port_restriction* pr = calloc(1, sizeof(struct http_proxy));
        pr->port = htons(port);
        pr->next = root_pr;
        root_pr = pr;
    }

    config->pr = root_pr;
    return true;
}

bool http_proxy(char **words, int wordslen, struct sp_config* config)
{
    if(strcmp(words[0], "http_proxy"))
        return false;

    if(wordslen != 3 && wordslen != 5)
        return false;

    struct inaddr_storage addr_storage;
    sa_family_t family = get_inaddr(words[1], &addr_storage);
    
    long port;
    if(!stol(words[2], &port) || port < 0 || 65535 < port)
        return false;

    struct http_proxy* hp = calloc(1, sizeof(struct http_proxy));
    hp->domain = strndup(words[1], MAX_DOMAIN_LEN);
    hp->port = strndup(words[2], MAX_DOMAIN_LEN);
    hp->addr_family = family;
    
    if(hp->addr_family == AF_INET) {
        hp->inaddr = calloc(1, sizeof(struct in_addr));
        memcpy(hp->inaddr, &addr_storage, sizeof(struct in_addr));
    } else if(hp->addr_family == AF_INET6) {
        hp->in6addr = calloc(1, sizeof(struct in6_addr));
        memcpy(hp->in6addr, &addr_storage, sizeof(struct in6_addr));
    }
    
    if(wordslen == 5) {
        hp->user = strndup(words[3], MAX_USER_LEN);
        hp->pass = strndup(words[4], MAX_USER_LEN);
    }
    
    hp->next = config->hp;
    config->hp = hp;
    
    return true;
}


/*
 *
 * public functions
 *
 */
static sp_config_t cfg = {0};

const sp_config_t* get_config(void)
{
    return &cfg;
}

bool read_config(const char *filepath)
{
    FILE * fd;

    fd = fopen(filepath, "r");
    if (fd == NULL)
        return false;

    char line[BUF_SIZE];
    while(fgets(line, BUF_SIZE, fd)) {
        char *words[MAX_WORDS] = {NULL};
        int word_count = get_words(line, words, MAX_WORDS);

        if(empty(words, word_count))
            continue;
        if(comment(words, word_count))
            continue;
        if(always_direct(words, word_count, &cfg))
            continue;
        if(port_restriction(words, word_count, &cfg))
            continue;
        if(http_proxy(words, word_count, &cfg))
            continue;
    }

    fclose(fd);

    if(cfg.hp == NULL)
        return false;

    return true;
}
