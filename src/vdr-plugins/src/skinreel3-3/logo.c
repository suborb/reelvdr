/*
 * logo.c: 'ReelNG' skin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 */

#include <sstream>

#include "common.h"
#include "bitmap.h"
#include "config.h"
#include "logo.h"

#include <vdr/plugin.h>

cReelLogoCache ReelLogoCache(0);

cReelLogoCache::cReelLogoCache(unsigned int cacheSizeP) :cacheSizeM(cacheSizeP), bitmapM(NULL)
{
#ifdef HAVE_IMAGEMAGICK
  bmpImage = new cBitmap(8, 8, 1);
#endif
}

cReelLogoCache::~cReelLogoCache()
{
#ifdef HAVE_IMAGEMAGICK
  delete bmpImage;
#endif

  // let's flush the cache
  Flush();
}

bool cReelLogoCache::Resize(unsigned int cacheSizeP)
{
  debug("cPluginSkinReel::Resize(%d)", cacheSizeP);
  // flush cache only if it's smaller than before
  if (cacheSizeP < cacheSizeM) {
    Flush();
  }
  // resize current cache
  cacheSizeM = cacheSizeP;
  return true;
}

bool cReelLogoCache::DrawEventImage(const cEvent *Event, int x, int y, int w, int h, int c, cBitmap *bmp)
{
  if (Event == NULL || bmp == NULL)
    return false;

  std::stringstream strFilename;
  strFilename << ReelConfig.GetImagesDir() << '/' << Event->EventID() << '.' << ReelConfig.GetImageExtension();
  int rc = DrawImage(strFilename.str().c_str(), x, y, w, h, c, bmp);
  return rc;
}

bool cReelLogoCache::DrawRecordingImage(const cRecording *Recording, int x, int y, int w, int h, int c, cBitmap *bmp)
{
  if (Recording == NULL || bmp == NULL)
    return false;

  std::stringstream strFilename;
  strFilename << Recording->FileName() << '/' << RECORDING_COVER << '.' << ReelConfig.GetImageExtension();
  int rc = DrawImage(strFilename.str().c_str(), x, y, w, h, c, bmp);
  return rc;
}

bool cReelLogoCache::DrawImage(const char *fileNameP, int x, int y, int w, int h, int c, cBitmap *bmp)
{
  if (fileNameP== NULL || bmp == NULL)
    return false;

  struct stat stbuf;
  if (lstat(fileNameP, &stbuf) != 0) {
    error("cPluginSkinReel::LoadImage(%s) FILE NOT FOUND", fileNameP);
    bitmapM = NULL;
    return false;
  }

#ifdef HAVE_IMAGEMAGICK
  bitmapM = NULL;
  return image.DrawImage(fileNameP, x, y, w, h, c, bmp);
#else
  int rc = LoadXpm(fileNameP, w, h);
  if (rc)
    bmp->DrawBitmap(x, y, *bitmapM); //TODO?
  return rc;
#endif
}

bool cReelLogoCache::LoadChannelLogo(const cChannel *Channel)
{
  if (Channel == NULL)
    return false;

  bool fFoundLogo = false;
  char *strChannelID = ReelConfig.useChannelId && !Channel->GroupSep() ? strdup(*Channel->GetChannelID().ToString()) : NULL;
  const char *logoname = ReelConfig.useChannelId && !Channel->GroupSep() ? strChannelID : Channel->Name();
  if (logoname) {
    char *filename = (char *)malloc(strlen(logoname) + 20 /* should be enough for folder */);
    if (filename == NULL) return false;
    strcpy(filename, "hqlogos/");
    strcat(filename, logoname);
    if (!(fFoundLogo = Load(filename, ChannelLogoWidth, ChannelLogoHeight, false))) {
      strcpy(filename, "logos/");
      strcat(filename, logoname);
      if (!(fFoundLogo = Load(filename, ChannelLogoWidth, ChannelLogoHeight, false))) {
        error("cPluginSkinReel::LoadChannelLogo: LOGO \"%s.xpm\" NOT FOUND in %s/[hq]logos", logoname, ReelConfig.GetLogoDir());
        fFoundLogo = Load("hqlogos/no_logo", ChannelLogoWidth, ChannelLogoHeight); //TODO? different default logo for channel/group?
      }
    }
    free(filename);
    free(strChannelID);
  }
  return fFoundLogo;
}

bool cReelLogoCache::LoadSymbol(const char *fileNameP)
{
  return Load(fileNameP, SymbolWidth, SymbolHeight);
}

bool cReelLogoCache::LoadIcon(const char *fileNameP)
{
  return Load(fileNameP, IconWidth, IconHeight);
}

bool cReelLogoCache::Load(const char *fileNameP, int w, int h, bool fLogNotFound)
{
  if (fileNameP == NULL)
    return false;

  std::string strFilename;
  strFilename = ReelConfig.GetLogoPath(fileNameP, ".xpm");

  debug("cPluginSkinReel::Load(%s)", strFilename.c_str());
  // does the logo exist already in map
  std::map < std::string, cBitmap * >::iterator i = cacheMapM.find(strFilename.c_str());
  if (i != cacheMapM.end()) { /** yes - cache hit! */
    debug("cPluginSkinReel::Load() CACHE HIT!");
    if (i->second == NULL) { /** check if logo really exist */
      debug("cPluginSkinReel::Load() EMPTY");
      // empty logo in cache
      return false;
    }
    bitmapM = i->second;
  } else { /** no - cache miss! */
    debug("cPluginSkinReel::Load() CACHE MISS!");
    // try to load xpm logo
    if (!LoadXpm(strFilename.c_str(), w, h, fLogNotFound))
      return false;
    if (cacheSizeM) { /** check if cache is active */
      // update map
      if (cacheMapM.size() >= cacheSizeM) {
        // cache full - remove first
        debug("cPluginSkinReel::Load() DELETE");
        if (cacheMapM.begin()->second != NULL) {
          // logo exists - delete it
          cBitmap *bmp = cacheMapM.begin()->second;
          DELETENULL(bmp);
        }
        cacheMapM.erase(cacheMapM.begin()); /** erase item */
      }
      // insert logo into map
      debug("cPluginSkinReel::Load() INSERT(%s)", strFilename.c_str());
      cacheMapM.insert(std::make_pair(strFilename.c_str(), bitmapM));
    }
    // check if logo really exist
    if (bitmapM == NULL) {
      debug("cPluginSkinReel::Load() EMPTY");
      // empty logo in cache
      return false;
    }
  }
  return true;
}

cBitmap & cReelLogoCache::Get(void)
{
  return *bitmapM;
}

bool cReelLogoCache::LoadXpm(const char *fileNameP, int w, int h, bool fLogNotFound)
{
  if (fileNameP == NULL)
    return false;

  struct stat stbuf;
  cBitmap *bmp = new cBitmap(1, 1, 1);

  // create absolute filename
  debug("cPluginSkinReel::LoadXpm(%s)", fileNameP);
  // check validity
  if ((lstat(fileNameP, &stbuf) == 0) && bmp->LoadXpm(fileNameP)) {
    if ((bmp->Width() <= w) && (bmp->Height() <= h)) {
      debug("cPluginSkinReel::LoadXpm(%s) LOGO FOUND", fileNameP);
      // assign bitmap
      bitmapM = bmp;
      return true;
    } else {
      // wrong size
      error("cPluginSkinReel::LoadXpm(%s) LOGO HAS WRONG SIZE %d/%d (%d/%d)", fileNameP, bmp->Width(), bmp->Height(), w, h);
    }
  } else {
    // no xpm logo found
    if (fLogNotFound)
      error("cPluginSkinReel::LoadXpm(%s) LOGO NOT FOUND", fileNameP);
  }

  delete bmp;
  bitmapM = NULL;
  return false;
}

bool cReelLogoCache::Flush(void)
{
  debug("cPluginSkinReel::Flush()");
  // check if map is empty
  if (!cacheMapM.empty()) {
    debug("cPluginSkinReel::Flush() NON-EMPTY");
    // delete bitmaps and clear map
    for (std::map<std::string, cBitmap *>::iterator i = cacheMapM.begin(); i != cacheMapM.end(); i++) {
      delete((*i).second);
    }
    cacheMapM.clear();
    // nullify bitmap pointer
    bitmapM = NULL;
  }
  return true;
}

// vim:et:sw=2:ts=2:
