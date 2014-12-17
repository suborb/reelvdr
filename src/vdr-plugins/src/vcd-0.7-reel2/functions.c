/*
 * VCD Player plugin for VDR
 * functions.c: Functions for handling VCDs
 *
 * See the README file for copyright information and how to reach the author.
 *
 * This code is distributed under the terms and conditions of the
 * GNU GENERAL PUBLIC LICENSE. See the file COPYING for details.
 *
 */


#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include "functions.h"
#include "setup.h"

__u8 bcd_to_bin(__u8 bcd)
{
  return 10 * (bcd >> 4) + (bcd & 0x0F);
}

int bcdmsf_to_lba(struct cdrom_msf0 msf0)
{
  return bcd_to_bin(msf0.frame)
     + CD_FRAMES * ( bcd_to_bin(msf0.second)
        + CD_SECS * bcd_to_bin(msf0.minute) )
     - CD_MSF_OFFSET;
}

struct cdrom_msf0 lba_to_msf(int lba)
{
  struct cdrom_msf0 msf0;
  msf0.frame = lba % CD_FRAMES;
  lba /= CD_FRAMES;
  msf0.second = lba % CD_SECS;
  msf0.minute = lba / CD_SECS;
  return msf0;
}


// --- cVcd ------------------------------------------------------------------

// const char *cVcd::deviceName = "/dev/cdrom";
//cVcd *cVcd::vcdInstance = NULL;

cVcd *cVcd::getVCD(void)
{
  if (!vcdInstance)
     new cVcd;
  return vcdInstance;
}

cVcd::cVcd(void)
{
  cdromCdio = 0;
  cdrom = 0;
  vcdInstance = this;
  deviceName = strdup("/dev/cdrom");
}

cVcd::cVcd(const char *DeviceName)
{
  cdromCdio = 0;
  cdrom = 0;
  vcdInstance = this;
  deviceName = strdup(DeviceName);
}

cVcd::~cVcd()
{
  Close();
}

int cVcd::Command(int Cmd)
{
    int result = -1;
    if (medium == MEDIUM_DEVICE)
    {
        int f;
        if ((f = open(deviceName, O_RDONLY | O_NONBLOCK)) > 0) {
            result = ioctl(f, Cmd, 0);
            close(f);
            }
    }
    return result;
}

bool cVcd::DriveExists(void)
{
    return medium != MEDIUM_DEVICE || access(deviceName, F_OK) == 0;
}

bool cVcd::DiscOk(void)
{
    if (medium == MEDIUM_DEVICE)
    {
        return Command(CDROM_DRIVE_STATUS) == CDS_DISC_OK;
    }
    else
    {
        return true;
    }
}

void cVcd::Eject(void)
{
    if (medium == MEDIUM_DEVICE)
    {
        if (vcdInstance)
            vcdInstance->Close();
        Command(CDROMEJECT);
    }
}

bool cVcd::Open(void)
{
    if (medium == MEDIUM_BIN_CUE)
    {
        if (!cdromCdio)
        {
            cdromCdio = cdio_open_bincue(filePath.c_str());
            return true;
        }
        return false;
    }
    else // Medium == MEDIUM_DEVICE
    {
        if (!cdrom) {
            cdrom = open(deviceName, O_RDONLY | O_NONBLOCK);
            SetDriveSpeed(VcdSetupData.DriveSpeed);
            return true;
            }
        return false;
    }
}

void cVcd::Close(void)
{
    if (medium == MEDIUM_DEVICE)
    {
        if (cdrom)
        {
            SetDriveSpeed(0);
            close(cdrom);
        }
        cdrom = 0;
    }
    else
    {
        if (cdromCdio)
        {
            cdio_destroy(cdromCdio);
        }
        cdromCdio = 0;
    }
    tracks = 0;
    memset(vcdEntry, 0, sizeof(vcdEntry));
    memset(&vcdInfo, 0, sizeof(vcdInfo));
    memset(&vcdEntries, 0, sizeof(vcdEntries));
    memset(&vcdLot, 0, sizeof(vcdLot));
    memset(&vcdPsd, 0, sizeof(vcdPsd));
}

void cVcd::SetDriveSpeed(int DriveSpeed)
{
    if (medium == MEDIUM_DEVICE)
    {
        ioctl(cdrom, CDROM_SELECT_SPEED, DriveSpeed);
    }
}

int cVcd::readTOC(__u8 format)
{
    if (medium == MEDIUM_DEVICE)
    {
        struct cdrom_tochdr tochdr;
        int i;
        tracks = -1;
        if (ioctl(cdrom, CDROMREADTOCHDR, &tochdr) == -1)
            return -1;
        for (i=tochdr.cdth_trk0; i<=tochdr.cdth_trk1; i++) {
            vcdEntry[i-1].cdte_track = i;
            vcdEntry[i-1].cdte_format = format;
            if (ioctl(cdrom, CDROMREADTOCENTRY, &vcdEntry[i-1]) == -1)
            return -1;
            }
        vcdEntry[tochdr.cdth_trk1].cdte_track = CDROM_LEADOUT;
        vcdEntry[tochdr.cdth_trk1].cdte_format = format;
        if (ioctl(cdrom, CDROMREADTOCENTRY, &vcdEntry[tochdr.cdth_trk1]) == -1)
            return -1;
        tracks = tochdr.cdth_trk1 - 1;
        return tochdr.cdth_trk1 - 1;
    }
    else
    {
        int trk0 = cdio_get_first_track_num(cdromCdio);
        int trk1 = trk0 + cdio_get_num_tracks(cdromCdio) - 1;
        for (int i = trk0; i <= trk1; ++i)
        {
            int lsn = cdio_get_track_lsn(cdromCdio, i);
            vcdEntry[i-1].cdte_addr.lba = lsn;
        }
        vcdEntry[trk1].cdte_addr.lba = cdio_get_track_lsn(cdromCdio, CDIO_CDROM_LEADOUT_TRACK);
        tracks = trk1 - 1;
        return trk1 - 1;
    }
}

bool cVcd::readSectorRaw(int lba, void *sect)
{
    if (medium == MEDIUM_DEVICE)
    {
        struct cdrom_msf0 msf0 = lba_to_msf(lba+CD_MSF_OFFSET);
        memcpy(sect, &msf0, sizeof(struct cdrom_msf0));
        if (ioctl(cdrom, CDROMREADRAW, sect) < 0)
        {
            return false;
        }
        return true;
    }
    else
    {
        ::printf("ERROR: cVcd::readSectorRaw()!\n");
        return false;
    }
}

bool cVcd::readSectorXA21(int lba, void *data)
{
    if (medium == MEDIUM_DEVICE)
    {
        struct cdsector_xa21 sect;
        if (readSectorRaw(lba, &sect) == false)
            return false;
        memcpy(data, sect.data, sizeof(sect.data));
        return true;
    }
    else
    {
        if (cdio_read_mode2_sector(cdromCdio, data, lba, false))
        {
            return false;
        }
        return true;
    }
}

bool cVcd::readSectorXA22(int lba, void *data)
{
    if (cdio_read_mode2_sector(cdromCdio, data, lba, true))
    {
        return false;
    }
    return true;
}

bool cVcd::isLabel(void)
{
  if (!vcdInstance)
     return false;
  if (strncmp(vcdInfo.system_id,"VIDEO_CD",8)==0)
     return true;
  if (strncmp(vcdInfo.system_id,"SUPERVCD",8)==0)
     return true;
  if (strncmp(vcdInfo.system_id,"HQ-VCD  ",8)==0)
     return true;
  return false;
}
