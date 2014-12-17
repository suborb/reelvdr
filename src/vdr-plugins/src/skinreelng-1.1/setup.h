/*
 * setup.h: 'ReelNG' skin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 */

#include "common.h"
#include "config.h"
#include <vdr/plugin.h>

class cPluginSkinReelSetup : public cMenuSetupPage {
private:
  cReelConfig data;

  virtual void Setup(void);
  void AddCategory(const char *Title);
protected:
  virtual eOSState ProcessKey(eKeys Key);
  virtual void Store(void);
public:
    cPluginSkinReelSetup(void);
};

class cMenuSetupSubMenu : public cOsdMenu {
protected:
  cReelConfig *data;
  virtual eOSState ProcessKey(eKeys Key);
  virtual void Set(void) = 0;
public:
  cMenuSetupSubMenu(const char *Title, cReelConfig *Data);
};

class cMenuSetupGeneral : public cMenuSetupSubMenu {
private:
  const char *showRemainingTexts[3];
  const char *showRecSizeTexts[3];
  const char *statusLineModeTexts[3];
protected:
  virtual eOSState ProcessKey(eKeys Key);
  void Set(void);
public:
  cMenuSetupGeneral(cReelConfig *Data);
};

class cMenuSetupLogos : public cMenuSetupSubMenu {
private:
  const char *showSymbolsTexts[4];
  const char *resizeImagesTexts[3];
protected:
  virtual eOSState ProcessKey(eKeys Key);
  void Set(void);
public:
  cMenuSetupLogos(cReelConfig *Data);
};

#ifdef HAVE_FREETYPE
class cMenuSetupTTF : public cOsdMenu {
private:
  FontInfo *data;
  int nFont;
  int nWidth;
  int nSize;
#if VDRVERSNUM < 10504 && !defined(REELVDR)
  const char **availTTFs;
  int nMaxTTFs;
#else // VDRVERSNUM >= 10504
  cStringList *fontList;
#endif // VDRVERSNUM < 10504
protected:
  virtual eOSState ProcessKey(eKeys Key);
  void Set(void);
  void Store(void);
public:
#if VDRVERSNUM < 10504 && !defined(REELVDR)
  cMenuSetupTTF(FontInfo *fontinfo);
#else // VDRVERSNUM >= 10504
  cMenuSetupTTF(FontInfo *fontinfo, cStringList* fontList);
#endif // VDRVERSNUM < 10504
};

#endif

class cMenuSetupFonts : public cMenuSetupSubMenu {
private:
#ifdef HAVE_FREETYPE
#if VDRVERSNUM >= 10504 || defined(REELVDR)
  cStringList fontNames;
  cStringList fontMonoNames;
#endif
#endif

protected:
  virtual eOSState ProcessKey(eKeys Key);
  void Set(void);
public:
  cMenuSetupFonts(cReelConfig *Data);
  virtual ~cMenuSetupFonts(void);
};

// vim:et:sw=2:ts=2:
