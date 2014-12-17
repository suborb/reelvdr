

#ifndef __DEBUG_MACROS__H
#define __DEBUG_MACROS__H

#include<cstdio>
#include<iostream>



#if 1


/* just a helper for code location */
#define LOC std::cout << "DEBUG: \t\t(" << __FILE__ << ":" << __LINE__ << ")  "<<__PRETTY_FUNCTION__<<"  ";

#define HERE LOC printf("\n");

/* macro using var args */
#define DEBUG_PRINT(fmt,...) LOC printf(fmt,__VA_ARGS__);

/* macro for general debug print statements. */
#define DEBUG_VAL(text) LOC std::cout << text << std::endl;

/* macro that dumps a variable name and its actual value */
#define DEBUG_VAR(text) LOC std::cout << (#text) << "=" << text << std::endl;



#else

/* when debug isn't defined all the macro calls do absolutely nothing */
#define DEBUG_PRINT(fmt,...)
#define DEBUG_VAL(text)
#define DEBUG_VAR(text)
#define HERE 

#endif
#endif

