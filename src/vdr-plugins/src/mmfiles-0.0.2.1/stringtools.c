#include "stringtools.h"


std::string StringOverlap(const std::string& str1, const std::string& str2)
{
    size_t idx=0;
    size_t minLen = std::min( str1.length(), str2.length());

    for (idx=0; idx < minLen; ++idx)
        if( str1.at(idx) != str2.at(idx) ) break;

    if (idx > 0)
        return str1.substr(0,idx);

    return "";
}

