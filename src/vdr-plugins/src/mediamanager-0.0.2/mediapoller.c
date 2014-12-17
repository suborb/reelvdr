/*
 * mediapoller.c:
 *
 * parts of this file are adapted from hal-project
 * addon-storage.c : Poll storage devices for media changes
 *
 * Copyright (C) 2004 David Zeuthen, <david@fubar.dk>
 * and
 * probe-volume.c : Probe for volume type (filesystems etc.)
 *
 * Copyright (C) 2004 David Zeuthen, <david@fubar.dk>
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id$
 */

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <mntent.h>
#include <unistd.h>
#include <linux/cdrom.h>
#include <scsi/sg.h>
#include <sys/ioctl.h>
#include <vdr/tools.h>

#include "mediapoller.h"

#define MEDIA_STATUS_UNKNOWN	0
#define MEDIA_STATUS_NO_MEDIA	1
#define MEDIA_STATUS_GOT_MEDIA	2
#define MEDIA_STATUS_EXTERN_USE	3

#define CAP_MEDIA_CHANGE_UNKOWN 0
#define CAP_MEDIA_CHANGE_TRUE	1
#define CAP_MEDIA_CHANGE_FALSE	2

//#define DEBUG_D 1
//#define DEBUG_I 1
#include "mediadebug.h"

cMediaPoller::cMediaPoller() {
	first = true;
	ismounted = false;
	trayisopen = false;
	support_media_changed = CAP_MEDIA_CHANGE_UNKOWN;
	mediastatus = MEDIA_STATUS_UNKNOWN;
	mediatype = mtNoDisc;
}

cMediaPoller::~cMediaPoller()
{
}

eMediaType cMediaPoller::poll(const char *DeviceFile, const char *RealDevice)
{
	check_media(DeviceFile, RealDevice);
	return mediatype;
}

bool cMediaPoller::OpenCloseTray(const char *DeviceFile)
{
	int status;
	int fd = open(DeviceFile, O_RDONLY | O_NONBLOCK);
	if(fd < 0) {
		DBG_E("[cMediaPoller]: cannot open device %s", DeviceFile)
		return false;
	}
/*
	int drive, disc_info;
	drive = ioctl(fd, CDROM_DRIVE_STATUS, CDSL_CURRENT);
	disc_info = ioctl(fd, CDROM_DISC_STATUS, CDSL_CURRENT);
printf("OpenCloseTray: open: %s drive %d disc %d\n", trayisopen ? "yes": "no", drive, disc_info);
*/

	if(!trayisopen || (mediatype > mtNoDisc)) {
		status = ioctl(fd, CDROMEJECT);
		if(status == 0) {
			trayisopen = true;
			close(fd);
			return true;
		}
	}
	status = ioctl(fd, CDROMCLOSETRAY);
printf("Status closetray %d\n", status);
	close(fd);
	if(status == 0) {
		trayisopen = false;
		return true;
	}
	return false;	
}

bool cMediaPoller::EjectDisc(const char *DeviceFile)
{
	int status;
	bool ret = false;
	int fd = open(DeviceFile, O_RDONLY | O_NONBLOCK);
	if(fd < 0) {
		DBG_E("[cMediaPoller]: cannot open device %s", DeviceFile)
		return ret;
	}
	status = ioctl(fd, CDROMEJECT);

	if(status == 0) {
		ret = true;
		trayisopen = true;
	} else
		DBG_E("[cMediaPoller]: eject failed %d", status)

	close(fd);
	return ret;
}

bool cMediaPoller::isMounted(const char *RealDevice)
{
	struct mntent *ent = NULL;
	FILE *m = setmntent(_PATH_MOUNTED, "r");
	bool ret = false;
	if(m) {
		while((ent = getmntent(m)) != NULL) {
			if(strcmp(RealDevice, ent->mnt_fsname) == 0) {
				ret = true;
				break;
			}
		}
		endmntent(m);
	}

	return ret;
}

/* adapted from hal
 * addon-storage.c : Poll storage devices for media changes
 *
 * Copyright (C) 2004 David Zeuthen, <david@fubar.dk>
 * and
 * probe-volume.c : Probe for volume type (filesystems etc.)
 *
 * Copyright (C) 2004 David Zeuthen, <david@fubar.dk>
*/
void cMediaPoller::check_media(const char *DeviceFile, const char *RealDevice)
{
	int fd;
	int drive;
	bool got_media = false;
	bool check_for_disc_only = mediastatus > MEDIA_STATUS_NO_MEDIA;

	if(check_for_disc_only) {
DBG_D("[cMediaPoller]: check for disc only")
		fd = open(DeviceFile, O_RDONLY | O_NONBLOCK);
		if(fd < 0) {
DBG_D("[cMediaPoller]: check for disc only: Open failed")
			return;
		}
	} else {
		fd = open(DeviceFile, O_RDONLY | O_NONBLOCK | O_EXCL);
		if(fd < 0 && errno == EBUSY) {
DBG_D("[cMediaPoller]: Device busy")
			/* this means the disc is mounted or some other app,
			 * like a cd burner, has already opened O_EXCL */
			if(!isMounted(RealDevice)) { //not our disc
				mediatype = mtExternUse;
				mediastatus = MEDIA_STATUS_EXTERN_USE;
DBG_D("[cMediaPoller]: Extern Use of disc")
				return;
			}
			/* check if mounted by us */
			if(ismounted)
				return;
			/* Handle VDR restart */
			if(first)
				fd = open(DeviceFile, O_RDONLY | O_NONBLOCK);
		}
		first = false;
		if(fd < 0) {
			DBG_E("[cMediaPoller]: open failed for %s: %s",
								DeviceFile, strerror (errno));
			return;
		}
		if(support_media_changed == CAP_MEDIA_CHANGE_UNKOWN) {
			int cap = ioctl(fd, CDROM_GET_CAPABILITY, 0);
			if(cap < 0) {
				DBG_E("[cMediaPoller]: Cannot get DriveCapabilities");
				support_media_changed = CAP_MEDIA_CHANGE_FALSE;
			} else {
				if(cap & CDC_MEDIA_CHANGED)
					support_media_changed = CAP_MEDIA_CHANGE_TRUE;
				else
					support_media_changed = CAP_MEDIA_CHANGE_FALSE;
			}
		}
	}

	/* Check if a disc is in the drive */
	drive = ioctl(fd, CDROM_DRIVE_STATUS, CDSL_CURRENT);
	switch(drive) {
		case CDS_NO_INFO:
		case CDS_NO_DISC:
		case CDS_TRAY_OPEN:
		case CDS_DRIVE_NOT_READY:
			break;
		case CDS_DISC_OK:
			/* some CD-ROMs report CDS_DISK_OK even with an open
			 * tray; if media check has the same value two times in
			 * a row then this seems to be the case and we must not
			 * report that there is a media in it. */
			if(support_media_changed &&
			    ioctl(fd, CDROM_MEDIA_CHANGED, CDSL_CURRENT) && 
			    ioctl(fd, CDROM_MEDIA_CHANGED, CDSL_CURRENT)) {
DBG_D("[cMediaPoller]: No media change")
			} else {
				trayisopen = true;
				got_media = true;
			}
			break;
		case -1:
			DBG_E("[cMediaPoller]: CDROM_DRIVE_STATUS failed: %s\n", strerror(errno));
			break;
		default:
			break;
	}

	if(got_media && check_for_disc_only) {
DBG_D("[cMediaPoller]: check for disc only: already have disc")
		close(fd);
		return;
	} else if(!got_media && check_for_disc_only) {
DBG_D("[cMediaPoller]: check for disc only: disc removed")
	}

//DBG_D("cMediaPoller poll got media status %d",mediastatus)
	switch(mediastatus) {
		case MEDIA_STATUS_GOT_MEDIA:
			if(!got_media) {
DBG_D("[cMediaPoller]: media removed")
				mediatype = mtNoDisc;
				trayisopen = true;
			}
			break;
		case MEDIA_STATUS_UNKNOWN:
		case MEDIA_STATUS_EXTERN_USE:
		case MEDIA_STATUS_NO_MEDIA:
			if(got_media) {
				int disc_info;
				disc_info = ioctl(fd, CDROM_DISC_STATUS, CDSL_CURRENT);
DBG_D("[cMediaPoller]: got media")
DBG_D("[cMediaPoller]: Doing probe-volume for %s", DeviceFile)
				switch(disc_info) {
					case CDS_AUDIO:		/* audio CD */
						mediatype = mtAudio;
DBG_D("[cMediaPoller]: Disc in %s has audio", DeviceFile)
						break;
					case CDS_MIXED:		/* mixed mode CD */
						mediatype = mtAudio;
DBG_D("[cMediaPoller]: Disc in %s has audio+data", DeviceFile)
						break;
					case CDS_DATA_1:	/* data CD */
					case CDS_DATA_2:
					case CDS_XA_2_1:
					case CDS_XA_2_2:
DBG_D("[cMediaPoller]: Disc in %s has data", DeviceFile)
						mediatype = advanced_disc_detect(fd, DeviceFile);
						break;
					case CDS_NO_INFO:	/* blank or invalid CD */
						mediatype = mtBlank;
DBG_D("[cMediaPoller]: Disc in %s is blank", DeviceFile)
						break;
					default:		/* should never see this */
						mediatype = mtNoDisc;
						DBG_E("[cMediaPoller]: Disc in %s returned unknown CDROM_DISC_STATUS", DeviceFile);
						break;
				}					
			}
			break;
		default:
			break;
	}

	close(fd);

	/* update our current status */
	if(got_media)
		mediastatus = MEDIA_STATUS_GOT_MEDIA;
	else
		mediastatus = MEDIA_STATUS_NO_MEDIA;
}

eMediaType cMediaPoller::advanced_disc_detect(int fd, const char *DeviceFile)
{
	/* the discs block size */
	unsigned short bs; 
	/* the path table size */
	unsigned short ts;
	/* the path table location (in blocks) */
	unsigned int tl;
	/* length of the directory name in current path table entry */
	unsigned char len_di = 0;
	/* the number of the parent directory's path table entry */
	unsigned int parent = 0; 
	/* filename for the current path table entry */
	char dirname[256];
	/* our position into the path table */
	int pos = 0; 
	/* the path table record we're on */
	int curr_record = 1; 
	/* loop counter */
	int i; 
	
	eMediaType ret = mtData;
	lseek (fd, 0, SEEK_SET);

	/* read the block size */
	lseek (fd, 0x8080, SEEK_CUR);
	if (read (fd, &bs, 2) != 2)
	{
		DBG_E("[cMediaPoller]: Advanced probing on %s failed while reading block size", DeviceFile);
		goto out;
	}

	/* read in size of path table */
	lseek (fd, 2, SEEK_CUR);
	if (read (fd, &ts, 2) != 2)
	{
		DBG_E("[cMediaPoller]: Advanced probing on %s failed while reading path table size", DeviceFile);
		goto out;
	}

	/* read in which block path table is in */
	lseek (fd, 6, SEEK_CUR);
	if (read (fd, &tl, 4) != 4)
	{
		DBG_E("[cMediaPoller]: Advanced probing on %s failed while reading path table block", DeviceFile);
		goto out;
	}

	/* seek to the path table */
	lseek (fd, bs * tl, SEEK_SET);

	/* loop through the path table entriesi */
	while (pos < ts)
	{
		/* get the length of the filename of the current entry */
		if (read (fd, &len_di, 1) != 1)
		{
			DBG_E("[cMediaPoller]: Advanced probing on %s failed, cannot read more entries", DeviceFile);
			break;
		}

		/* get the record number of this entry's parent
		   i'm pretty sure that the 1st entry is always the top directory */
		lseek (fd, 5, SEEK_CUR);
		if (read (fd, &parent, 2) != 2)
		{
			DBG_E("[cMediaPoller]: Advanced probing on %s failed, couldn't read parent entry", DeviceFile);
			break;
		}
		
		/* read the name */
		if (read (fd, dirname, len_di) != len_di)
		{
			DBG_E("[cMediaPoller]: Advanced probing on %s failed, couldn't read the entry name", DeviceFile);
			break;
		}
		dirname[len_di] = 0;

		/* strcasecmp is not POSIX or ANSI C unfortunately */
		i=0;
		while (dirname[i]!=0)
		{
			dirname[i] = (char)toupper (dirname[i]);
			i++;
		}

DBG_D("[cMediaPoller]: Advanced probing dir %s",dirname)
		if (endswith(dirname, ".REC")) {
DBG_D("[cMediaPoller]: Disc in %s contains VDR recordings", DeviceFile)
			ret = mtVDRdata;
			break;
		}
		/* if we found a folder that has the root as a parent, and the directory name matches 
		   one of the special directories then set the properties accordingly */
		if (parent == 1)
		{
			if (!strcmp (dirname, "VIDEO_TS"))
			{
				ret = mtVideoDvd;
DBG_D("[cMediaPoller]: Disc in %s is a Video DVD", DeviceFile)
				break;
			}
			else if (!strcmp (dirname, "VCD"))
			{
				ret = mtVcd;
DBG_D("[cMediaPoller]: Disc in %s is a Video CD", DeviceFile)
				break;
			}
			else if (!strcmp (dirname, "SVCD"))
			{
				ret = mtSvcd;
DBG_D("[cMediaPoller]: Disc in %s is a Super Video CD", DeviceFile)
				break;
			}
		}

		/* all path table entries are padded to be even, 
		   so if this is an odd-length table, seek a byte to fix it */
		if (len_di%2 == 1)
		{
			lseek (fd, 1, SEEK_CUR);
			pos++;
		}

		/* update our position */
		pos += 8 + len_di;
		curr_record++;
	}

out:
	/* go back to the start of the file */
	lseek (fd, 0, SEEK_SET);
	return ret;
}

/* end adapted from hal */
