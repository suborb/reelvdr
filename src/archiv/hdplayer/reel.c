// reel.c

#include "reel.h"

#ifdef REEL_ENABLE_ASSERTIONS

#include <stdio.h>
#include <stdlib.h>

void ReelDebugAssertFailed(char const *file, int line)
{
    fprintf(stderr, "### Debug assertion failed: %s [%d]\n", file, line);
    abort();
}

#endif // def REEL_ENABLE_ASSERTIONS
