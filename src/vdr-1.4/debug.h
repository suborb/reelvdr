
#ifndef DEBUG_H
#define DEBUG_H
#include <assert.h>

#ifdef DBG
  #undef DBG
  #undef DERR
  #undef DLOG
  #undef PRINTF
#endif


#if defined DEBUG
    #ifdef PLUGIN_NAME_I18N
        #define CONTEXT PLUGIN_NAME_I18N
    #else
        #define CONTEXT "VDR, " __FILE__
    #endif

    // standard debug macro
    #define DDD(x...) { printf("["CONTEXT": %s, %d, %s]: ",__FILE__, __LINE__, __FUNCTION__); \
                         printf(x); printf("\n"); }

    // standard error macro (red background)
    #define DERR(x...) { printf("\033[0;37m\033[1;41m["CONTEXT"]: "x); printf("\033[0m\n"); }

    // special macro: logs to syslog AND console
    #define DLOG(x...) { printf("["CONTEXT"]: "x); printf("\n"); dsyslog("["CONTEXT"]: "x); }

    // for automatic conversion of printf's - i.e. use sed s/" printf"/" PRINTF"/
    #define PRINTF(x...) printf("["CONTEXT"]: "x)
#else
    #define DDD(x...)
    #define DERR(x...)
    #define DLOG(x...)
    #define PRINTF(x...)
#endif

//only here for compatibility reasons, don't use any more
#define ERR "Error"

#define DBG          ""

#endif /* DEBUG_H */
