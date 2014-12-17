/*
 * dpkg Plugin for VDR
 * menu.h: OSD menus
 *
 * See the README file for copyright information and how to reach the author.
 *
 * This code is distributed under the terms and conditions of the
 * GNU GENERAL PUBLIC LICENSE. See the file COPYING for details.
 *
 */


#ifndef __DPKG_MENUMAIN_H
#define __DPKG_MENUMAIN_H

#include <apt-front/progresscallback.h>
#include <apt-front/manager.h>
#include <apt-front/cache/cache.h>
#include <apt-front/cache/entity/package.h>
#include <apt-front/sources.h>

#include <vdr/osdbase.h>
#include <vdr/tools.h>
#include <vdr/menu.h>
#if VDRVERSNUM < 10727
#include "i18n.h"
#endif

#include "dpkg.h"

static const char *VERSION = "0.0.3";

#ifdef RBMINI
#define INET_TIMEOUT 30000
#else
#define INET_TIMEOUT 10000
#endif

class cPkgItem:public cOsdItem {
	private:
		int mMode;
	public:
		aptFront::cache::entity::Package mPkg;
		cPkgItem(aptFront::cache::entity::Package fPkg, int fMode = 0);
		~cPkgItem();
		virtual int Compare(const cListObject &ListObject) const;
		int HasShortDescr();
		std::string GetShortDescr();
		void Show(void);
		virtual eOSState ProcessKey(eKeys Key);
}; // cPkgItem

class cAptThread;
class cAptProgress;
class cMyManager;
class cSourceMenu;
class cMainMenu:public cOsdMenu {
	friend class cAptThread;
	friend class cAptProgress;
	friend class cMyManager;
	friend class cSourceMenu;
private:
	cAptThread *mApt;
	aptFront::cache::Cache *mCache;
	int mCommit;
	int mReboot;
	int mMode;
	int mState;
	int mHideStatus;
	int mDisplay;
	cMutex mMutex;
	aptFront::Sources mSourcesList;
	cString mResult1;
	cString mResult2;
	cString mStatus;
	cString mStatusProgress;
	cStringList     mStatusInfo;
	cList<cPkgItem> mReelItems;
	cList<cPkgItem> mReelOptItems;
	void Update();
	void Upgrade();
	void AddPackageItem(cPkgItem *fItem);
	void ShowReelPackages();
	void ShowReelOptPackages();
	void ShowSources();
	void ShowReboot();
	void ShowExec(const char *txt, const char *cmd);
	void ProcessUpdate();
	cString GetProgress(int pos, int max);
public:
	cMainMenu(int fMode, const char *fTitle = NULL);
	~cMainMenu(void);
	virtual eOSState ProcessKey(eKeys Key);
	virtual void Display(void);
	virtual void Clear(void);
	void AddItem(const char *fState, const char *fText);
	void AddItem(cOsdItem *fItem);
	void ClearItems();
	void ClearState();
	void SetStatusText(const char *fText, const char *fProgress);
	void SetStatusLock(const char *fText);
	void SetStatusUnLock();
	void SetState(int fCommit);
	void SetReboot();
}; // cMainMenu

#define APT_IDLE   0
#define APT_UPDATE 1
#define APT_SEARCH 2
#define APT_COMMIT 3
#define APT_EXEC   4
class cAptProgress: public aptFront::ProgressCallback {
public:
	cMainMenu*  mMenu;
	int mContinue;
	int mMode;
	int mGood;
	cStringList mProgressList;
	cStringList mDoneList;
	cStringList mFailList;
	cStringList mErrorList;
	cTimeMs     mTimeout;
public:
	cAptProgress(cMainMenu*fMenu);
	virtual void IMSHit(pkgAcquire::ItemDesc &Itm);
	virtual void Fetch(pkgAcquire::ItemDesc &Itm);
	virtual void Done(pkgAcquire::ItemDesc &Itm);
	virtual void Fail(pkgAcquire::ItemDesc &Itm);
	virtual void UpdatePulse(double FetchedBytes, double CurrentCPS, unsigned long CurrentItems);
	virtual bool Pulse(pkgAcquire *Owner);
	void SetMode(int fMode);
}; // cAptProgress

class cAptThread: public cThread {
	private:
		cMainMenu* mMenu;
		int mAction;
		cAptProgress *mProgress;
		const char *mCmd;
	public:
		cAptThread(cMainMenu*fMenu);
		~cAptThread(void);
		void Stop();
		int IsIdle();
		virtual void DoUpdate();
		virtual void DoCommit();
		virtual void DoExecute(const char *Cmd);
		virtual int Abort();
		virtual void Action(void);
}; // cAptThread

class cMyManager:public aptFront::Manager, public cThread {
	private:
		cMainMenu*mMenu;
		int mStatusRead;
		char mStatusBuff[1024];
		unsigned int mStatusPos;
	public:
		cStringList mErrorList;
	public:
		cMyManager(cMainMenu*fMenu);
		~cMyManager();
		virtual void Action(void);
		void myCommit();
		char *getInfo();
}; // cMyManager

class cSourceMenu:public cOsdMenu {
	private:
		cMainMenu *mMenu;
		int mFailed;
	public:
		cSourceMenu(cMainMenu *fMenu);
		~cSourceMenu(void);
		virtual eOSState ProcessKey(eKeys Key);
		void ShowHelp();
		void ReadFile();
		void WriteFile();
}; // cSourceMenu

#define SOURCE_CHARS_MAX 1000
class cSourceItem:public cMenuEditStrItem {
	public:
		aptFront::Sources::Entry mEntry;
		cSourceItem(const char *fLine);
		~cSourceItem();
		virtual eOSState ProcessKey(eKeys Key);
		char mLine[SOURCE_CHARS_MAX+1];
}; // cSourceItem

#endif //__DPKG_MENUMAIN_H
