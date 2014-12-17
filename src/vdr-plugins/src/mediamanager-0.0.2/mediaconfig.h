/*
 * mediaconfig.h:
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id$
 */

#ifndef _MEDIA_CONFIG_H
#define _MEDIA_CONFIG_H


class cMediaConfig {
  private:
	char *device_file;
	char *mount_point;
	char *real_device_file;
	char *archiv_dir;
	int autoplay;

	static cMediaConfig *config;
	void SetRealDeviceName(const char *DeviceFile);
	void SetMountPoint(const char *DeviceFile);
	cMediaConfig();
  public:
	~cMediaConfig();
	static cMediaConfig& GetConfig(void);
	void SetDeviceFile(const char *DeviceFile);
	void SetArchivDir(const char *Dir);
	void SetAutoPlay(int On) { autoplay = On; }

	const char *GetDeviceFile(void) { return device_file; }
	const char *GetRealDeviceFile(void) { return real_device_file; }
	const char *GetMountPoint(void) { return mount_point; }
	const char *GetArchivDir(void) { return archiv_dir; }
	bool AutoPlay(void) { return autoplay ? true : false; }
};
#endif
