#include "libsheerproxy.h"
#include "dns.h"
#include "uthash.h"

#include <string.h>
#include <stdbool.h>
#include <pthread.h>
#include <arpa/inet.h>

#define MAX_OFFSET 0x0FFFFFFE
#define ERR_OFFSET 0x10000000

/*
 * address translator
 */
static const uint32_t IPV4_CLASS_E_INIT = 0xF0000000;

static const struct in6_addr IPV6_RESERVED_FOR_DOC_INIT = \
                {{{ 0x20, 0x01, 0x0d, 0xb8, 0x00, 0x00, 0x00, 0x00, \
                    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }}};

static bool is_ipv4classe(const struct in_addr *sin_addr)
{
    return (ntohl(sin_addr->s_addr) & IPV4_CLASS_E_INIT) == IPV4_CLASS_E_INIT;
}

static struct in_addr offset_to_ipv4classe(uint32_t offset)
{
    struct in_addr ipv4;
    ipv4.s_addr = htonl(IPV4_CLASS_E_INIT | offset);
    return ipv4;
}

static uint32_t ipv4classe_to_offset(const struct in_addr *sin_addr)
{
    return (~IPV4_CLASS_E_INIT) & ntohl(sin_addr->s_addr);
}

static bool is_v6reserved(const struct in6_addr* sin6_addr)
{
    return (memcmp(sin6_addr, &IPV6_RESERVED_FOR_DOC_INIT, 12) == 0);
}

static struct in6_addr offset_to_v6reserved(uint32_t offset)
{
    struct in6_addr ipv6 = IPV6_RESERVED_FOR_DOC_INIT;
    *((uint32_t *)(&ipv6.s6_addr[12])) = htonl(offset);
    return ipv6;
}

static uint32_t v6reserved_to_offset(const struct in6_addr* sin6_addr)
{
    return ntohl(*((uint32_t *)(&sin6_addr->s6_addr[12])));
}

static struct sp_addr* spaddr_from(uint32_t offset)
{
    struct sp_addr *spaddr;
    
    spaddr = calloc(1, sizeof(struct sp_addr));
    
    spaddr->sin_addr = offset_to_ipv4classe(offset);
    spaddr->sin6_addr = offset_to_v6reserved(offset);
    
    return spaddr;
}

/*
 * offset manager
 */
static uint32_t next_offset = MAX_OFFSET;
static pthread_rwlock_t offset_lock = PTHREAD_RWLOCK_INITIALIZER;

uint32_t new_offset(void)
{
    if (pthread_rwlock_wrlock(&offset_lock) != 0)
        return ERR_OFFSET;
    
    uint32_t offset = next_offset++;
    if(next_offset > MAX_OFFSET)
        next_offset = 0;
    
    pthread_rwlock_unlock(&offset_lock);
    
    return offset;
}

/*
 * record
 */
struct record_t {
    char *node;          /* node name */
    uint32_t offset;     /* address offset */
    UT_hash_handle hh_node;  /* handle for record_by_node hash table */
    UT_hash_handle hh_offset;  /* handle for record_by_offset hash table */
};

static struct record_t* newrecord(const char *node, uint32_t offset)
{
    struct record_t *new_record = calloc(1, sizeof(struct record_t));
    new_record->node = strndup(node, MAX_DOMAIN_LEN);
    new_record->offset = offset;
    
    return new_record;
}

static void freerecord(struct record_t *record)
{
    free(record->node);
    free(record);
}

/*
 * Hashmap
 */
static struct record_t *hm_record_by_node = NULL;
static struct record_t *hm_record_by_offset = NULL;
static pthread_rwlock_t hm_node_lock = PTHREAD_RWLOCK_INITIALIZER;
static pthread_rwlock_t hm_offset_lock = PTHREAD_RWLOCK_INITIALIZER;

struct record_t* find_record_by_node(const char *node)
{
    if (!node)
        return NULL;

    if (pthread_rwlock_rdlock(&hm_node_lock) != 0)
        return NULL;
    
    struct record_t *record = NULL;
    HASH_FIND( hh_node, hm_record_by_node, node, strlen(node), record );
    
    pthread_rwlock_unlock(&hm_node_lock);
    
    return record;
}

struct record_t* find_record_by_offset(uint32_t offset)
{
    if (pthread_rwlock_rdlock(&hm_offset_lock) != 0)
        return NULL;
    
    struct record_t *record = NULL;
    HASH_FIND( hh_offset, hm_record_by_offset, &offset, sizeof(offset), record );
    
    pthread_rwlock_unlock(&hm_offset_lock);
    
    return record;
}

struct record_t* add_record(struct record_t *record)
{
    if (record == NULL)
        return NULL;

    if (pthread_rwlock_wrlock(&hm_node_lock) != 0)
        return NULL;
    
    if (pthread_rwlock_wrlock(&hm_offset_lock) != 0) {
        pthread_rwlock_unlock(&hm_node_lock);
        return NULL;
    }
    
    uint32_t offset = record->offset;
    HASH_ADD_KEYPTR( hh_node, hm_record_by_node, record->node, strlen(record->node), record );
    HASH_ADD( hh_offset, hm_record_by_offset, offset, sizeof(offset), record );
    
    pthread_rwlock_unlock(&hm_node_lock);
    pthread_rwlock_unlock(&hm_offset_lock);
    
    return record;
}

struct record_t* delete_record(struct record_t *record)
{
    if (record == NULL)
        return NULL;

    if (pthread_rwlock_wrlock(&hm_node_lock) != 0)
        return NULL;
    
    if (pthread_rwlock_wrlock(&hm_offset_lock) != 0) {
        pthread_rwlock_unlock(&hm_node_lock);
        return NULL;
    }

    HASH_DELETE( hh_node, hm_record_by_node, record);
    HASH_DELETE( hh_offset, hm_record_by_offset, record);

    pthread_rwlock_unlock(&hm_node_lock);
    pthread_rwlock_unlock(&hm_offset_lock);
    
    return record;
}

/*
 * helper logic
 */
static char* querynode_common(uint32_t offset)
{
    char *node = NULL;
    struct record_t *record = find_record_by_offset(offset);

    if (record) {
        node = strndup(record->node, MAX_DOMAIN_LEN);
    }

    if (!node) {
        return NULL;
    }
    
    return node;
}


/*
 *
 * public functions
 *
 */
struct sp_addr* querybyname(const char *node)
{
    if(!node)
        return NULL;

    // return in_addr if node exists in dns.
    struct record_t *exist_record = find_record_by_node(node);
    if(exist_record)
        return spaddr_from(exist_record->offset);

    // allocate offset for new record.
    uint32_t offset = new_offset();
    if(offset == ERR_OFFSET)
        return NULL;
        
    // remove old dns instance against cyclic offset.
    struct record_t *old_record = find_record_by_offset(offset);
    if(old_record) {
        struct record_t *deleted_record = delete_record(old_record);
        freerecord(old_record);
        if(!deleted_record) {
            return NULL;
        }
    }
    
    // create new dns instance.
    struct record_t *new_record = newrecord(node, offset);
    if(!add_record(new_record)) {
        freerecord(new_record);
        return NULL;
    }
    
    return spaddr_from(new_record->offset);
}

char* querybyaddr(const struct in_addr *sin_addr)
{
    if (!sin_addr || !is_ipv4classe(sin_addr))
        return NULL;

    return querynode_common( ipv4classe_to_offset(sin_addr) );
}

char* querybyaddr6(const struct in6_addr *sin6_addr)
{
    if (!sin6_addr || !is_v6reserved(sin6_addr))
        return NULL;
    
    return querynode_common( v6reserved_to_offset(sin6_addr) );
}
