#include "helpers.h"

// --- OSD Message Helpers -----------------------------------

void OsdMsg(eMessageType Type, const char *Msg)
{
#if VDRVERSNUM >= 10307
  Skins.Message(Type, Msg);
#else
  switch(Type)
  {
    case mtInfo:
      Interface->Info(Msg);
      break;
    case mtWarning: //Was muss da hin??
      Interface->Error(Msg);
      break;
    case mtError:
      Interface->Error(Msg);
      break;
  }
#endif
}

void ChangeChars(char *name, char *chars)
{
  char seek;
  char replace;

  double len = strlen(chars);

  if ((len / 2) != floor(len / 2))
    len--;

  for(int i = 0; i < (int)len; i += 2)
  {
    seek = chars[i];
    replace = chars[i + 1];
    name = strreplace(name, seek, replace);
  }
}

void StrRepeat(char *input, int count, char *dest)
{
  for(int i = 0; i < count; i++)
    strcat(dest, input);
}

bool RegIMatch(const char *string, const char *pattern)
{
  if(!string || !pattern)
    return false;

  bool ret = false;
  regex_t reg;

  if(!regcomp(&reg, pattern, REG_EXTENDED | REG_ICASE | REG_NOSUB))
  {
    if(!regexec(&reg, string, 0, NULL, 0))
      ret = true;
  }

  regfree(&reg);

  return ret;
}

// --- cTokenizer ---------------------------------------

void cTokenizer::Tokenize(bool trim)
{
  Clear();
  MYDEBUG("String wird in Token zerlegt");
  char *buffer = NULL;
  char *token = NULL;
  char *tok_pointer;

  buffer = strdup(String);
  if(trim)
    buffer = stripspace(buffer);

  for(token = strtok_r(buffer, Delim, &tok_pointer);
      token;
      token = strtok_r(NULL, Delim, &tok_pointer))
  {
    MYDEBUG("Token gefunden: %s", token);
    if(!trim)
      Add(new cToken(token));
    else
      Add(new cToken(stripspace(token)));
  }

  free(buffer);
}

// --- cFileInfo ---------------------------------------------

cFileInfo::cFileInfo(char *file)
{
  MYDEBUG("FileInfo: %s", file);
  File = (file && !isempty(file)) ? strdup(file) : NULL;

  if(File && File[strlen(File) - 1] == '/')
      File[strlen(File) - 1] = '\0';

  buffer = NULL;

  if(isExists())
    stat64(File, &Info);

  size = 0;
}

cFileInfo::~ cFileInfo(void)
{
  free(File);
  free(buffer);
}

char *cFileInfo::Path(void)
{
  FREENULL(buffer);

  char *p = strrchr(File, '/');
  if(p)
  {
    int len = strlen(File) - strlen(p) + 1;
    buffer = (char*)malloc(len);
    strn0cpy(buffer, File, len);
  }

  MYDEBUG("FileInfo: Pfad: %s", buffer);

  return buffer;
}

char *cFileInfo::FileName(void)
{
  FREENULL(buffer);

  char *p = strrchr(File, '/');
  if(p)
  {
    p++;
    buffer = strdup(p);
  }

  MYDEBUG("FileInfo: FileName: %s", buffer);

  return buffer;

}

char *cFileInfo::FileNameWithoutExt(void)
{
  char *ext = NULL;
  char *filename = NULL;
  
  if(Extension())
     ext = strdup(Extension());
  if(FileName())
    filename = strdup(FileName());

  FREENULL(buffer);
  
  if(ext && filename)
  {
    int len = strlen(filename) - strlen(ext) + 1;
    buffer = (char*)malloc(len);
    strn0cpy(buffer, filename, len);
  }
  else if(filename)
    buffer = strdup(filename);

  free(ext);
  free(filename);

  MYDEBUG("FileInfo: FileNameWithoutExt: %s", buffer);

  return buffer;
}

char *cFileInfo::Extension(bool withPoint)
{
  FREENULL(buffer);

  char *p = strrchr(File, '.');
  if(p)
  {
    if(!withPoint)
      p++;
    if(!isempty(p))
      buffer = strdup(p);
  }

  MYDEBUG("FileInfo: Extension: %s", buffer);
  
  return buffer;
}

eFileInfo cFileInfo::Type(void)
{
  switch(Info.st_mode & S_IFMT)
  {
    case S_IFDIR:
      return tDir;
      break;
    case S_IFREG:
    case S_IFLNK:
      return tFile;
      break;
    default:
      return tUnknown;
      break;
  }
}

unsigned long long int cFileInfo::SizeByte(void)
{
  cFileList *list = NULL;
  cFileListItem *item = NULL;
  cFileInfo *info = NULL;

  if(!size)
  {
    switch(Type())
    {
      case tFile:
        size = Info.st_size;
        break;
      case tDir:
        list = new cFileList();
        list->OptFilterType(tFile);
        if(list->Load(File, true))
        {
          item = list->First();
          while(item)
          {
            info = new cFileInfo(item->Value());
            size += info->SizeByte();
            DELETENULL(info);
            item = list->Next(item);
          }
        }
        delete(list);
        break;
      default:
        break;
    }
  }

  return size;
}

double cFileInfo::SizeKByte(int decimals)
{
  double size = (double)SizeByte() / 1024;
  
  for(int i = 0; i < decimals; i++)
    size *= 10;

  size = (double)(unsigned int)(size + 0.5);

  for(int i = 0; i < decimals; i++)
    size /= 10;

  return size;
}

double cFileInfo::SizeMByte(int decimals)
{
  double size = (double)SizeByte() / 1024 / 1024;

  for(int i = 0; i < decimals; i++)
    size *= 10;

  size = (double)(unsigned int)(size + 0.5);

  for(int i = 0; i < decimals; i++)
    size /= 10;

  return size;
}

double cFileInfo::SizeGByte(int decimals)
{
  double size = (double)SizeByte() / 1024 / 1024 / 1024;

  for(int i = 0; i < decimals; i++)
    size *= 10;

  size = (double)(unsigned int)(size + 0.5);

  for(int i = 0; i < decimals; i++)
    size /= 10;

  return size;
}

bool cFileInfo::isReadable(void)
{
  if(getuid() == 0)
    return true;

  if(Info.st_uid == getuid() &&
     Info.st_mode & S_IRUSR)
    return true;

  if(Info.st_gid == getgid() &&
     Info.st_mode & S_IRGRP)
    return true;

  if(Info.st_mode & S_IROTH)
    return true;

  return false;
}

bool cFileInfo::isWriteable(void)
{
  if(getuid() == 0)
    return true;

  if(Info.st_uid == getuid() &&
     Info.st_mode & S_IWUSR)
    return true;

  if(Info.st_gid == getgid() &&
     Info.st_mode & S_IWGRP)
    return true;

  if(Info.st_mode & S_IWOTH)
    return true;

  return false;
}

bool cFileInfo::isExecutable(void)
{
  if(getuid() == 0 &&
     (Info.st_mode & S_IXUSR ||
      Info.st_mode & S_IXGRP ||
      Info.st_mode & S_IXOTH))
    return true;

  if(Info.st_uid == getuid() &&
     Info.st_mode & S_IXUSR)
    return true;

  if(Info.st_gid == getgid() &&
     Info.st_mode & S_IXGRP)
    return true;

  if(Info.st_mode & S_IXOTH)
    return true;

  return false;
}

bool cFileInfo::isExists(void)
{
  return !access(File, F_OK);
}

// --- cFileCMD -------------------------------------------------

bool cFileCMD::Del(char *file)
{
  cFileInfo *info = new cFileInfo(file);
  cFileList *list = NULL;
  cFileListItem *item = NULL;
  bool ret = true;

  switch(info->Type())
  {
    case tDir:
      list = new cFileList();
      list->OptSort(sDesc);
      if(list->Load(file, true))
      {
        item = list->First();
        while(item)
        {
          DELETENULL(info);
          info = new cFileInfo(item->Value());
          if(info->Type() == tFile && remove(item->Value()) < 0)
            ret = false;
          if(info->Type() == tDir && rmdir(item->Value()) < 0)
            ret = false;
          item = list->Next(item);
        }
      } else
        ret = false;
      if(rmdir(file) < 0)
        ret = false;
      break;
    default:
      if(remove(file) < 0)
        ret = false;
      break;
  }

  delete(info);
  delete(list);

  return ret;
}

bool cFileCMD::DirIsEmpty(char *file)
{
  bool ret = false;

  cFileList *list = new cFileList();
  if(list->Load(file, true) && !list->Count())
    ret = true;
  delete(list);

  return ret;
}

// --- cFileList -------------------------------

cFileListItem::cFileListItem(char *dir, char *file)
{
  if(file && !isempty(file) && dir && !isempty(dir))
  {
    if(dir[strlen(dir) - 1] == '/')
      asprintf(&File, "%s%s", dir, file);
    else
      asprintf(&File, "%s/%s", dir, file);
  }
  else
    File = NULL;

  if(File && File[strlen(File) - 1] == '/')
    File[strlen(File) - 1] = '\0';
}

cFileListItem::cFileListItem(char *file)
{
  File = file ? strdup(file) : NULL;

  if(File && File[strlen(File) - 1] == '/')
    File[strlen(File) - 1] = '\0';
}

cFileList::cFileList(void)
{
  Dir = NULL;
  Sub = false;

  Includes = NULL;
  Excludes = new cFileOptList();
  Excludes->Add(new cFileOptListItem("^\\.{1,2}$"));
  Protect = false;

  SortMode = sNone;
  SortToFilename = false;
  Type = tNone;
}

cFileList::~cFileList(void)
{
  free(Dir);

  if(!Protect)
  {
    delete(Includes);
    delete(Excludes);
  }
}

bool cFileList::Load(char *dir, bool withsub)
{
  if(dir && isempty(dir))
    return false;

  if(!dir && !Dir)
    return false;

  Clear();

  if(dir)
  {
    FREENULL(Dir);
    Dir = strdup(dir);
    if(Dir[strlen(Dir) - 1] == '/')
      Dir[strlen(Dir) - 1] = '\0';
  }

  if(withsub)
    Sub = withsub;

  return Read(Dir, Sub);
}

bool cFileList::Read(char *dir, bool withsub)
{
  bool ret = false;
  char *buffer = NULL;

  struct dirent *DirData = NULL;

  cReadDir Dir(dir);
  if(Dir.Ok())
  {
    while((DirData = Dir.Next()) != NULL)
    {
      if(CheckIncludes(dir, DirData->d_name)  &&
         !CheckExcludes(dir, DirData->d_name) &&
         CheckType(dir, DirData->d_name, Type))
        SortIn(dir, DirData->d_name);
      if(withsub &&
         CheckType(dir, DirData->d_name, tDir) &&
         !RegIMatch(DirData->d_name, "^\\.{1,2}$"))
      {
        asprintf(&buffer, "%s/%s", dir, DirData->d_name);
        Read(buffer, withsub);
        FREENULL(buffer);
      }
    }
    ret = true;
  }

  return ret;
}

void cFileList::SortIn(char *dir, char *file)
{
  cFileListItem *Item = First();
  cFileInfo *ItemInfo = NULL;
  char *NewItem = NULL;
  cFileInfo *NewItemInfo = NULL;

  switch(SortMode)
  {
    case sNone:
      Add(new cFileListItem(dir, file));
      break;
    case sAsc:
    case sDesc:
      if(Item)
      {
        asprintf(&NewItem, "%s/%s", dir, file);
        NewItemInfo = SortToFilename ? new cFileInfo(NewItem) : NULL;
        while(Item)
        {
          if(SortToFilename)
          {
            DELETENULL(ItemInfo);
            ItemInfo = new cFileInfo(Item->Value());
          }

          if((SortMode == sAsc && !SortToFilename && strcasecmp(NewItem, Item->Value()) < 0) ||
             (SortMode == sDesc && !SortToFilename && strcasecmp(NewItem, Item->Value()) > 0))
            break;
          if((SortMode == sAsc && SortToFilename && strcasecmp(NewItemInfo->FileName(), ItemInfo->FileName()) < 0) ||
             (SortMode == sDesc && SortToFilename && strcasecmp(NewItemInfo->FileName(), ItemInfo->FileName()) > 0))
            break;

          Item = Next(Item);
        }
      }
      if(Item)
        Ins(new cFileListItem(dir, file), Item);
      else
        Add(new cFileListItem(dir, file));
      break;
    default:
      break;
  }

  delete(NewItemInfo);
  delete(ItemInfo);
  free(NewItem);
}

bool cFileList::CheckIncludes(char *dir, char *file)
{
  if(!Includes)
    return true;

  cFileOptListItem *iItem = Includes->First();
  while(iItem)
  {
    if(RegIMatch(file, iItem->Regex()) && CheckType(dir, file, iItem->FileType()))
      return true;

    iItem = Includes->Next(iItem);
  }

  return false;
}

bool cFileList::CheckExcludes(char *dir, char *file)
{
  if(!Excludes)
    return false;

  cFileOptListItem *eItem = Excludes->First();
  while(eItem)
  {
    if(RegIMatch(file, eItem->Regex()) && CheckType(dir, file, eItem->FileType()))
      return true;

    eItem = Excludes->Next(eItem);
  }

  return false;
}

bool cFileList::CheckType(char *dir, char *file, eFileInfo type)
{
  if(type == tNone)
    return true;

  bool ret = false;

  char *buffer = NULL;

  asprintf(&buffer, "%s/%s", dir, file);

  cFileInfo *info = new cFileInfo(buffer);
  if(info->Type() == type)
    ret = true;
  delete(info);

  free(buffer);

  return ret;
}

bool cFileList::DirIsEmpty(cFileListItem *item)
{
  bool ret = true;

  cFileInfo *fInfo = new cFileInfo(item->Value());

  if(fInfo->Type() == tDir)
  {
    cFileList *fList = new cFileList();

    fList->Includes = Includes;
    fList->Excludes = Excludes;
    fList->Protect = true;

    fList->Load(item->Value());
    if(fList->Count())
      ret = false;

    delete(fList);
  }

  delete(fInfo);

  return ret;
}

bool cFileList::DirIsIn(char *file, char *strings)
{
  bool ret = false;

  cFileInfo *fInfo = new cFileInfo(file);

  if(fInfo->Type() == tDir && strings && !isempty(strings))
  {
    cFileList *fList = new cFileList();
    char *buffer;

    cTokenizer *token = new cTokenizer(strings, "@");
    for(int i = 1; i <= token->Count(); i++)
    {
      asprintf(&buffer, "^%s$", token->GetToken(i));
      fList->OptInclude(buffer);
      free(buffer);
    }
    DELETENULL(token);

    fList->Load(file);
    if(fList->Count())
      ret = true;

    delete(fList);
  }

  delete(fInfo);

  return ret;
}
