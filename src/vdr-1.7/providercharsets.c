#include "providercharsets.h"

cProviderCharset::cProviderCharset(char *Name, char *Charset)
{
  charset = NULL;
  name = NULL;
  if(Charset)
    charset = strdup(Charset);
  if(Name)
    name = strdup(Name);
}

cProviderCharset::cProviderCharset(void)
{
  charset = NULL;
  name = NULL;
}

cProviderCharset::~cProviderCharset()
{
  free(charset);
  free(name);
}

bool cProviderCharset::Parse(const char *s)
{
  char *p = NULL;
  //printf("==== parse: %s ====\n", s);
  if((p = (char *) strchr(s, ';'))) {
     int len = strlen(s);
     int n = p-s;
     name = (char*)malloc(n+1);
     strncpy(name, s, n);
     name[n] = '\0';
     charset = (char*)malloc(len-n+1);
     strncpy(charset, s+n+1, len-n);
     charset[len-n] = '\0';
  }
  //printf("name: %s - charset: %s\n", name, charset);
  return name && charset;
}

cProviderCharsets ProviderCharsets;

cProviderCharsets::cProviderCharsets() {
  hasDefaultCharset = false;
  defaultCharset = NULL;
}

cProviderCharsets::~cProviderCharsets() {
  if(hasDefaultCharset)
    free(defaultCharset);
}

const char *cProviderCharsets::GetCharset(const char *providerName) const
{
  for (cProviderCharset *p = First(); p; p = Next(p)) {
      if (strcasecmp(p->Name(), providerName) == 0)
         return p->Charset();
      }
  if(hasDefaultCharset)
    return defaultCharset;
  return NULL;
}

bool cProviderCharsets::Load(const char *FileName, bool AllowComments, bool MustExist)
{
    cConfig<cProviderCharset>::Clear();
    if (FileName) {
       free(fileName);
       fileName = strdup(FileName);
       allowComments = AllowComments;
       }
    bool result = !MustExist;
    if (fileName && access(fileName, F_OK) == 0) {
       isyslog("loading %s", fileName);
       FILE *f = fopen(fileName, "r");
       if (f) {
          char *s;
          int line = 0;
          cReadLine ReadLine;
          result = true;
          while ((s = ReadLine.Read(f)) != NULL) {
                line++;
                if (allowComments) {
                   char *p = strchr(s, '#');
                   if (p)
                      *p = 0;
                   }
                stripspace(s);
                if (!isempty(s)) {
                   cProviderCharset *l = new cProviderCharset;
                   if (l->Parse(s)) {
                      if(!strcasecmp(l->Name(), "default")) {
                        hasDefaultCharset = true;
                        defaultCharset = strdup(l->Charset());
                      } else
                        Add(l);
                   } else {
                      esyslog("ERROR: error in %s, line %d", fileName, line);
                      delete l;
                      //RC: we do not want to exit vdr just because of a simple error
                      //result = false;
                      //break;
                      }
                   }
                }
          fclose(f);
          }
       else {
          LOG_ERROR_STR(fileName);
          result = false;
          }
       }
    if (!result)
       fprintf(stderr, "vdr: error while reading '%s'\n", fileName);
    return result;
  }

