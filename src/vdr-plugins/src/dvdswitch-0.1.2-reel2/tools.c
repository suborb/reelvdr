#include "tools.h"
#include "imagelist.h"
#include "setup.h"

cDirList::cDirList(void)
{
  char *buffer = NULL;

  OptExclude("^lost\\+found$"); // lost+found Dir
  OptExclude("^\\."); // hidden Files
  OptExclude("\\.sdel$"); // del Files
  OptExclude("\\.smove$"); // move Files

  cTokenizer *token = new cTokenizer(ImageList.GetDirContains(), "@");
  for(int i = 1; i <= token->Count(); i++)
  {
    asprintf(&buffer, "^%s$", token->GetToken(i));
    OptExclude(buffer);
    FREENULL(buffer);
  }

  OptSort((eFileList)DVDSwitchSetup.SortMode);
  OptFilterType(tDir);

  delete(token);
}

