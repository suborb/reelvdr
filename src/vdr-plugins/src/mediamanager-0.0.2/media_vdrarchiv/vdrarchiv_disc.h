/*
 * vdrarchiv_disc.h:
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id$
 */

#ifndef _MEDIA_VDRARCHIV_DISC_H
#define _MEDIA_VDRARCHIV_DISC_H

#include <vdr/i18n.h>
#include <vdr/tools.h>

class cMediaVDRArchivRecInfo {
  private:
	char *title;
	char *shorttext;
	char *description;
  public:
	cMediaVDRArchivRecInfo(const char *Filename);
	~cMediaVDRArchivRecInfo();
	const char *Title(void) { return title; }
	const char *ShortText(void) { return shorttext; }
	const char *Description(void) { return description; }
	//void SetData(const char *Title, const char *ShortText, const char *Description);
	bool Read(FILE *f);
};

class cMediaVDRArchivRec : public cListObject {
  private:
	char *rec_dir;
	char *date_time;
	char *name;
	char *title;
	bool has_info;
	bool has_summary;
	int level;
	int track;
	cMediaVDRArchivRecInfo *info;
  public:
	cMediaVDRArchivRec(const char *Directory, int Track, bool Info, bool Sum);
	~cMediaVDRArchivRec();

	const char *RecDir(void) { return rec_dir; }
	//const char *DateTime(void) { return date_time; }
	const char *Name(void) { return name; }
	bool HasInfo(void) { return has_info; }
	bool HasSummary(void) { return has_summary; }
	int Level(void) { return level; }
	int Track(void) { return track; }
	bool IsDirectory(int Level) { return (level - Level) != 0; }
	cMediaVDRArchivRecInfo *RecInfo(void) { return info; }
	const char *TitelByLevel(int Level);
};

class cMediaVDRArchivDisc : public cList<cMediaVDRArchivRec> {
  private:
  	int last_replayed;

	const char *mountdir;
	const char *archivdir;

	void RemoveDirRecursiv(const char *Directory);
	void RemoveDirs(void);
	bool MkDir(const char *Directory);
	bool MkSymlink(const char *From, const char *To);
	bool MkCopy(const char *From, const char *To);
	int SymlinkOrCopy(const char *From, const char *To);
  public:
	cMediaVDRArchivDisc();
	~cMediaVDRArchivDisc();

	void SetLastReplayed(int Last) { last_replayed = Last; }
	int LastReplayed(void) { return last_replayed; }

	void CleanUpArchiv(void);
	bool ScanDisc(void);
};

class cMediaVDRArchivRecDir : public cListObject {
  private:
  	char *dir;
  public:
	cMediaVDRArchivRecDir(const char *Directory);
	~cMediaVDRArchivRecDir();
	const char *GetDirectory(void) { return dir; }
};

class cMediaVDRArchivRecDirs : public cList<cMediaVDRArchivRecDir> {
  private:
	void ScanDisc(const char *Directory);
  public:
	cMediaVDRArchivRecDirs();
	~cMediaVDRArchivRecDirs();
	bool FindRecordingDirs(const char *Directory);
};

#endif
