/*
 * mediaconfig.c:
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id$
 */

#include <mntent.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "mediaconfig.h"

cMediaConfig::cMediaConfig()
{
	device_file = NULL;
	mount_point = NULL;
	real_device_file = NULL;
	archiv_dir = NULL;
	autoplay = 0;
}

cMediaConfig::~cMediaConfig()
{
	//if() free();
	if(device_file) free(device_file);
	if(mount_point) free(mount_point);
	if(real_device_file) free(real_device_file);
	if(archiv_dir) free(archiv_dir);
	config = NULL;
}

cMediaConfig *cMediaConfig::config = NULL;

cMediaConfig& cMediaConfig::GetConfig(void)
{
	if(config == NULL)
		config = new cMediaConfig();

	return *config;
}

void cMediaConfig::SetDeviceFile(const char *DeviceFile)
{
	if(device_file) free(device_file);
	if(mount_point) free(mount_point);
	if(real_device_file) free(real_device_file);
	device_file = NULL;
	mount_point = NULL;
	real_device_file = NULL;

	SetRealDeviceName(DeviceFile);
	if(real_device_file == NULL)
		return;

	SetMountPoint(DeviceFile);
	if(mount_point == NULL)
		return;

	device_file = strdup(DeviceFile);
}

void cMediaConfig::SetRealDeviceName(const char *DeviceFile)
{
	char resolved[4096];
	struct stat st;

	if(stat(DeviceFile, &st))
		return;
	if(lstat(DeviceFile, &st)) {
		real_device_file = strdup(DeviceFile);
		return;
	}
	if(realpath(DeviceFile, resolved))
		real_device_file = strdup(resolved);
}

void cMediaConfig::SetMountPoint(const char *DeviceFile)
{
	struct mntent *ent = NULL;
	FILE *m = setmntent(_PATH_MNTTAB, "r");
	if(m) {
		while((ent = getmntent(m)) != NULL) {
			if((strcmp(DeviceFile, ent->mnt_fsname) == 0)
					|| (strcmp(real_device_file, ent->mnt_fsname) == 0)) {
				mount_point = strdup(ent->mnt_dir);
				break;
			}
		}
		endmntent(m);
	}
}

void cMediaConfig::SetArchivDir(const char *Dir)
{
	if(Dir)
		archiv_dir = strdup(Dir);
}
