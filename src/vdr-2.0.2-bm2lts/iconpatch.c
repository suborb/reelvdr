
#include "iconpatch.h"

#include <langinfo.h>
#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

bool IsLangUtf8(void)
{
  char *CodeSet = NULL;
  if (setlocale(LC_CTYPE, ""))
     CodeSet = nl_langinfo(CODESET);
  else {
     char *LangEnv = getenv("LANG"); // last resort in case locale stuff isn't installed
     if (LangEnv) {
        CodeSet = strchr(LangEnv, '.');
        if (CodeSet)
           CodeSet++; // skip the dot
        }
     }

  if (CodeSet && strcasestr(CodeSet, "UTF-8") != 0)
     return true;

  return false;
}
