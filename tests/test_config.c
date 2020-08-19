#include "config.h"

#include "testutils.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>

#define MAX_WORDS 10

extern int get_words(char *, char **, int);

int main(int argc, char** argv) {
    
    /*
        get_words tests
     */
    {
        TEST("successful get_workds")
        char *words[MAX_WORDS] = {0};
        char line[] = "#always_direct  127.0.0.0 \t\t 8\n";
        
        int ret = get_words(line, words, MAX_WORDS);
        assert(ret == 3);
        assert(!strcmp(words[0], "#always_direct"));
        assert(!strcmp(words[1], "127.0.0.0"));
        assert(!strcmp(words[2], "8"));
    }
    
    {
        TEST("wordslen is less than actual words")
        char *words[4] = {0};
        char line[] = "1 2 3 4 5";
        
        int ret = get_words(line, words, 4);
        assert(ret == 4);
    }
    
    {
        TEST("zero words")
        char *words[MAX_WORDS] = {0};
        char line[] = "    ";
        
        int ret = get_words(line, words, MAX_WORDS);
        assert(ret == 0);
    }
    
    /*
        read_config tests
     */
    {
        TEST("successful read_config")
        char filepath[] = "./config_test.conf";
        bool ret = read_config(filepath);
        assert(ret == true);
        
        const struct sp_config* cfg = get_config();
        assert(cfg != NULL);
        
        assert(cfg->ad_v4 != NULL);
        assert(cfg->ad_v6 != NULL);
        assert(cfg->ad_domain != NULL);
        assert(cfg->pr != NULL);
        assert(cfg->hp != NULL);
    }

    TEST_DONE
    return 0;
}
