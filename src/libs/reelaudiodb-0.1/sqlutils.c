#include "sqlutils.h"


/* replace single quotes with \' */
std::string EscapeSingleQuotes(const std::string& str)
{
    std::string result;
    result.reserve(str.size()*2);

    std::string::const_iterator it = str.begin();
    while (it != str.end())
    {
        if (*it == '\'')
            result.push_back('\\');

        result.push_back(*it);
        ++it;
    }

    return result;
}


/* escape single quotes and enclose the given string in single quotes */
std::string QuoteString(const std::string& str)
{
    return std::string("\'") + EscapeSingleQuotes(str) + std::string("\'");
}
