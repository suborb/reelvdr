#ifndef SQL_UTILS_H
#define SQL_UTILS_H

#include <string>

/* replace single quotes with \' */
std::string EscapeSingleQuotes(const std::string& str);

/* escape single quotes and enclose the given string in single quotes */
std::string QuoteString(const std::string& str);

#endif // SQL_UTILS_H
