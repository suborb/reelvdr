/*
 * vdrarchiv_disc.c:
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id$
 */

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/stat.h>
#include <vdr/recording.h>
#include <vdr/tools.h>

#include "mediaconfig.h"
#include "media_vdrarchiv/vdrarchiv_disc.h"

//#define DEBUG_I 1
//#define DEBUG_D 1
#include "mediadebug.h"

//#define TEST_LOCAL 1

#define DATAFORMAT   "%4d-%02d-%02d.%02d%*c%02d*c"


cMediaVDRArchivRecInfo::cMediaVDRArchivRecInfo(const char *Filename)
{
	title = NULL;
	shorttext = NULL;
	description = NULL;

}

cMediaVDRArchivRecInfo::~cMediaVDRArchivRecInfo()
{
	if(title) free(title);
	if(shorttext) free(shorttext);
	if(description) free(description);
}

bool cMediaVDRArchivRecInfo::Read(FILE *f)
{
	cReadLine ReadLine;
	char *s;
	int line = 0;
	while((s = ReadLine.Read(f)) != NULL) {
		++line;
		char *t = skipspace(s + 1);
		switch (*s) {
			case 'T':
				title = strcpyrealloc(title, t);
				break;
			case 'S':
				shorttext = strcpyrealloc(shorttext, t);
				break;
			case 'D':
				strreplace(t, '|', '\n');
				description = strcpyrealloc(description, t);
				break;
			case 'C':
			case 'X':
			case 'V':
				break;
			default:
				DBG_E("ERROR: unexpected tag while reading EPG data: %s", s);
				return false;
				break;
		}
	}
	return true;
}

cMediaVDRArchivRec::cMediaVDRArchivRec(const char *Directory, int Track, bool Info, bool Sum)
{
	const char *archivdir = cMediaConfig::GetConfig().GetArchivDir();
	rec_dir = strdup(Directory);
	date_time = NULL;
	name = NULL;
	title = NULL;
	has_info = Info;
	has_summary = Sum;
	track = Track;
	level = 0;
	info = new cMediaVDRArchivRecInfo(rec_dir);

	char *p = strrchr(Directory, '/');
	Directory += strlen(archivdir) + 1;
	if(p) {
		struct tm t;
		if (5 == sscanf(p + 1, DATAFORMAT, &t.tm_year, &t.tm_mon, &t.tm_mday, &t.tm_hour, &t.tm_min)) {
			t.tm_year -= 1900;
			t.tm_sec = 0;
DBG_D("Time: %s\n is %02d.%02d.%02d %02d:%02d",Directory,
				t.tm_mday,t.tm_mon,t.tm_year%100,t.tm_hour,t.tm_min)

			asprintf(&date_time,"%02d.%02d.%02d\t%02d:%02d",
				t.tm_mday, t.tm_mon, t.tm_year % 100, t.tm_hour, t.tm_min);

			name = MALLOC(char, p - Directory + 1);
			strncpy(name, Directory, p - Directory);
			name[p - Directory] = 0;
DBG_D("Name before: |%s|", name)
			name = ExchangeChars(name, false);
DBG_D("Name after : |%s|", name)
			char *s = name;
			while(*++s) {
				if(*s=='~')
					level++;
			}
		}
		if(has_info) {
			char *InfoFileName = NULL;
			asprintf(&InfoFileName, "%s%s", rec_dir, "/info.vdr");
			FILE *f = fopen(InfoFileName, "r");
			if(f) {
				if(!info->Read(f))
					DBG_E("ERROR: EPG data problem in file %s", InfoFileName);
				fclose(f);
			} else 
				DBG_E("ERROR: Failed to open file %s", InfoFileName);
			free(InfoFileName);
		}
		if(isempty(info->Title()) && has_summary) {
			DBG_I("Reading summary.vdr not implemented yet");
		}
	}
}

const char *cMediaVDRArchivRec::TitelByLevel(int Level)
{
	int cur_level = Level;
	if(title)
		free(title);
	title = NULL;
//DBG_D("cMediaVDRArchivRec: rec_level %d cur_level %d", level,cur_level)

// level 0   1    2
// name  bla~blaa~blaaa
// title start

//DBG_D("Rec Name %d |%s|", cur_level, name)

	const char *s = name;
	if(cur_level > 0) { 
		while(*++s) {
			if(*s == '~')
				cur_level--;
			if(cur_level == 0)
				break;
		}
		s += 1;
	}
//DBG_D("Rec s |%s|", s)
	const char *e = s;
	while(*++e) {
		if(*e == '~')
			break;
	}
//DBG_D("Rec e |%s|", e)
	char *t = MALLOC(char,e - s + 1);
	strncpy(t, s, e - s);
	t[e - s] = 0;
//DBG_D("Rec t |%s|", t)
	if(IsDirectory(Level))
		asprintf(&title,  " \t%s", t);
	else
		asprintf(&title,  "%s\t%s", date_time, t);
	free(t);
	return title;
}

cMediaVDRArchivRec::~cMediaVDRArchivRec()
{
	free(rec_dir);
	free(date_time);
	free(name);
	if(title)
		free(title);
	delete(info);
}

cMediaVDRArchivDisc::cMediaVDRArchivDisc()
{
	last_replayed = -1;
	mountdir = cMediaConfig::GetConfig().GetMountPoint();
	archivdir = cMediaConfig::GetConfig().GetArchivDir();
}

cMediaVDRArchivDisc::~cMediaVDRArchivDisc()
{
	RemoveDirs();
}

void cMediaVDRArchivDisc::RemoveDirRecursiv(const char *Directory)
{
	cReadDir dir(Directory);
	if(dir.Ok()) {
DBG_D("[cMediaVDRArchivDisc]: Removing %s",Directory)
		struct dirent *e;
		while((e = dir.Next()) != NULL) {
			if(strcmp(e->d_name, ".") && strcmp(e->d_name, "..")) {
				char *buf = NULL;
				struct stat st;
				asprintf(&buf, "%s/%s", Directory, e->d_name);
				if(stat(buf, &st) == 0) {
					if(S_ISDIR(st.st_mode)) {
DBG_D("[cMediaVDRArchivDisc]: Directory %s already contains directory %s",Directory, e->d_name)
						RemoveDirRecursiv(buf);
					} else {
						if(remove(buf) < 0) {
DBG_E("[cMediaVDRArchivDisc]: Cannot remove %s",buf)
						}
					}
				}
				free(buf);
			}
		}
		if(remove(Directory) < 0) {
DBG_E("[cMediaVDRArchivDisc]: Cannot remove %s",Directory)
		} else {
DBG_D("[cMediaVDRArchivDisc]: Removed %s",Directory)
		}
	}
}

void cMediaVDRArchivDisc::RemoveDirs(void)
{
	cReadDir dir(archivdir);
	if(dir.Ok()) {
		struct dirent *e;
		while((e = dir.Next()) != NULL) {
			if(strcmp(e->d_name, ".") && strcmp(e->d_name, "..")) {
				char *buf = NULL;
				struct stat st;
				asprintf(&buf, "%s/%s", archivdir, e->d_name);
				if(stat(buf, &st) == 0) {
					RemoveDirRecursiv(buf);
				}
				free(buf);
			}
		}
	}
}

bool cMediaVDRArchivDisc::MkDir(const char *Directory)
{
	// for now use vdr MakeDirs
	return MakeDirs(Directory, true);
}

bool cMediaVDRArchivDisc::MkSymlink(const char *From, const char *To)
{
	bool ret = true;
	if(symlink(From, To) < 0) {
DBG_E("[cMediaVDRArchivDisc]: Failed to create symlink %s",To)
		ret = false;
	}
	return ret;
}

bool cMediaVDRArchivDisc::MkCopy(const char *From, const char *To)
{
	bool ret = true;
	FILE *in = fopen(From, "r");
	if(in) {
		FILE *out = fopen(To, "w");
		if(out) {
			char *s;
			cReadLine ReadLine;
			while((s = ReadLine.Read(in)) != NULL) {
				fprintf(out, "%s\n", s);
			}
			fclose(out);
		} else {
DBG_E("[cMediaVDRArchivDisc]: Failed to open %s",To)
			fclose(in);
			ret = false;
		}
		fclose(in);
	} else {
DBG_E("[cMediaVDRArchivDisc]: Failed to open %s",From)
		ret = false;
	}
	return ret;
}

#define MEDIA_MARKSFILE 	"marks.vdr"
#define MEDIA_RESUMEFILE	"resume.vdr"
#define MEDIA_INFO			"info.vdr"
#define MEDIA_OLDINFO		"summary.vdr"
#define MEDIA_INDEX 		"index.vdr"

#define HAS_REC 	1
#define HAS_INDEX	2
#define HAS_INFO	4
#define HAS_SUM 	8
#define HAS_MARK	16

#define ACTION_NONE 0
#define ACTION_COPY 1
#define ACTION_LINK 2

int cMediaVDRArchivDisc::SymlinkOrCopy(const char *From, const char *To)
{
	int ret = 0;
	cReadDir dir(From);
	struct dirent *e;
	while((e = dir.Next()) != NULL) {
		if(strcmp(e->d_name, ".") && strcmp(e->d_name, "..")) {
			char *from = NULL;
			char *to = NULL;
			int action = ACTION_LINK;
			int has_file = 0;
			asprintf(&from, "%s/%s", From, e->d_name);
			asprintf(&to, "%s/%s", To, e->d_name);
			if(strcmp(e->d_name, MEDIA_MARKSFILE) == 0) {
				action = ACTION_COPY;
				has_file = HAS_MARK;
			} else if(strcmp(e->d_name, MEDIA_RESUMEFILE) == 0) {
				action = ACTION_NONE;
			} else if(strcmp(e->d_name, MEDIA_INFO) == 0) {
				has_file = HAS_INFO;
			} else if(strcmp(e->d_name, MEDIA_OLDINFO) == 0) {
				has_file = HAS_SUM;
			} else if(strcmp(e->d_name, MEDIA_INDEX) == 0) {
				has_file = HAS_INDEX;
			} else {
				char *vdr = strdup(e->d_name);
				char *p = strchr(vdr, '.');
				if(p) {
					int num;
					*p = 0;
					num = atoi(vdr);
					if((num < 1) || (num > 255) || strcmp(p+1, "vdr"))
						action = ACTION_NONE;
					else
						has_file = HAS_REC;
					*p = '.';
				} else {
					action = ACTION_NONE;
				}
				free(vdr);
			}
			if(action == ACTION_COPY) {
				if(MkCopy(from, to))
					ret |= has_file;
			} else if(action == ACTION_LINK) {
				if(MkSymlink(from, to))
					ret |= has_file;
			}
			free(to);
			free(from);
		}
	}
	return ret;
}

bool cMediaVDRArchivDisc::ScanDisc(void)
{
	cMediaVDRArchivRecDirs *recDirs = new cMediaVDRArchivRecDirs();
#ifdef TEST_LOCAL
	bool ret = recDirs->FindRecordingDirs("/vdr_mount");
	int base_len = strlen("/vdr_mount");
#else
	bool ret = recDirs->FindRecordingDirs(mountdir);
	int base_len = strlen(mountdir);
#endif
	RemoveDirs();

	if(ret) {
		int num = 0;
		for(cMediaVDRArchivRecDir *dir = recDirs->First();
										dir; dir = recDirs->Next(dir)) {
			const char *p = dir->GetDirectory();
			char *buf = NULL;
			p += base_len;
			if(*p == '/')
				p++;
			asprintf(&buf, "%s/%s", archivdir, p);
//DBG_D("Got recdir %s",p)
			if(MkDir(buf)) {
				int files = SymlinkOrCopy(dir->GetDirectory(), buf);
				if(files & HAS_REC) {
					Add(new cMediaVDRArchivRec(buf, num,
								(files & HAS_INFO) == HAS_INFO,
								(files & HAS_SUM) == HAS_SUM));
					num++;
				} else {
DBG_E("[cMediaVDRArchivDisc]: Failed to create symlinks in %s",buf)
					RemoveDirRecursiv(buf);
				}
			} else {
DBG_E("[cMediaVDRArchivDisc]: Failed to create dir %s",buf)
			}
			free(buf);
		}
	}
	delete(recDirs);
	if(Count() == 0)
		return false;

	return true;
}

void cMediaVDRArchivDisc::CleanUpArchiv(void)
{
	RemoveDirs();
}

cMediaVDRArchivRecDir::cMediaVDRArchivRecDir(const char *Directory)
{
	dir = strdup(Directory);
}

cMediaVDRArchivRecDir::~cMediaVDRArchivRecDir()
{
	free(dir);
}

cMediaVDRArchivRecDirs::cMediaVDRArchivRecDirs()
{
}

cMediaVDRArchivRecDirs::~cMediaVDRArchivRecDirs()
{
}

void cMediaVDRArchivRecDirs::ScanDisc(const char *Directory)
{
	cReadDir dir(Directory);
	struct dirent *e;
	while((e = dir.Next()) != NULL) {
		if(strcmp(e->d_name, ".") && strcmp(e->d_name, "..")) {
			char *buf = NULL;
			struct stat st;
			asprintf(&buf, "%s/%s", Directory, e->d_name);
			if(stat(buf, &st) == 0) {
				if(S_ISDIR(st.st_mode)) {
					if(endswith(buf, ".rec")) {
						Add(new cMediaVDRArchivRecDir(buf));
					} else {
						ScanDisc(buf);
					}
				}
			}
			free(buf);
		}
	}
}

bool cMediaVDRArchivRecDirs::FindRecordingDirs(const char *Directory)
{
	ScanDisc(Directory);
	if(Count())
		return true;

	return false;
}

