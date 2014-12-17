
#ifndef STUPIDPROVIDERS_H
#define STUPIDPROVIDERS_H

#include "config.h"

class cProviderCharset : public cListObject {
  private:
    char *name;
    char *charset;
  public:
    cProviderCharset(void);
    cProviderCharset(char *name, char *charset);
    ~cProviderCharset();
    inline char *Charset() const { return charset; }
    inline char *Name() const { return name; }
    bool Parse(const char *s);
};

class cProviderCharsets : public cConfig<cProviderCharset> {
private:
  char *defaultCharset;
  bool hasDefaultCharset;
public:
  cProviderCharsets();
  ~cProviderCharsets();
  bool Load(const char *FileName = NULL, bool AllowComments = false, bool MustExist = false);
  const char *GetCharset(const char* providerName) const;
  };

extern cProviderCharsets ProviderCharsets;

#endif
