/*********************************************************
 * copied from setup-plugin
 *********************************************************/

#ifndef UTIL_H
#define UTIL_H

class Util
{
  public:
    Util();
    ~Util();
    enum Type
    { UNDEFINED, TEXT, NUMBER_TEXT, HEX, IP, BOOL, NUMBER, SELECTION };
    static char *Strdupnew(const char *str);
    static char *Strdupnew(const char *prefix, const char *str);
    static char *Strdupnew(const char *str, int size);
    static bool isBool(const char *string, bool & flag);
    static bool isBool(const char *string, int &flag);
    static bool isType(const char *string, Util::Type & typ);
    static bool isNumber(const char *string, int &number);
    static const char *typeToStr(Type type);
    static const char *boolToStr(bool val);

};

#endif
