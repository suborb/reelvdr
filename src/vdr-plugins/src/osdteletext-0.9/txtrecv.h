/***************************************************************************
 *       Copyright (c) 2003,2004 by Marcel Wiesweg                         *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef __TXTRECV_H                   
#define __TXTRECV_H

#include <vdr/status.h>
#include <vdr/receiver.h>
#include <vdr/thread.h>
#include <vdr/ringbuffer.h>

#include <stdio.h>
#include <unistd.h>

struct PageID {
   PageID() { page=subPage=0; }
   PageID(tChannelID id, int p, int s) { set(id, p, s); }
   void set(tChannelID id, int p, int s)
      { channel=id; page=p; subPage=s; }
   tChannelID channel;
   int page;
   int subPage;
};

struct StorageHandle {
public:
   StorageHandle() { handle=-1; }
   StorageHandle(const StorageHandle &s) { handle=s.handle; }
   StorageHandle(int h) { handle=h; }
   StorageHandle &operator=(int h) { handle=h; return *this; }
   StorageHandle &operator=(const StorageHandle &s) { handle=s.handle; return *this; }
   operator bool() const { return handle!=-1; }
   operator int() const { return handle; }
private:
   int handle;
};

class RootDir {
public:
   static void setRootDir(const char *);
   static const char *getRootDir();
protected:
   static const char *root;
};

class Storage : public RootDir {
public:
   virtual ~Storage();
   enum StorageSystem { StorageSystemLegacy, StorageSystemPacked };
   //must be called before the first call to instance()
   
   static Storage *CreateInstance(StorageSystem system, int storageLimit);
   
   //must be called before operation starts. Set all options (RootDir, maxStorage) before.
   virtual void cleanUp() = 0;
   
   virtual void getFilename(char *buffer, int bufLength, PageID page);
   virtual void prepareDirectory(tChannelID chan);
   
   virtual StorageHandle openForWriting(PageID page) = 0;
   virtual StorageHandle openForReading(PageID page, bool countAsAccess) = 0;
   virtual ssize_t write(const void *ptr, size_t size, StorageHandle stream) = 0;
   virtual ssize_t read(void *ptr, size_t size, StorageHandle stream) = 0;
   virtual void close(StorageHandle stream) = 0;
protected:
   Storage();
   int cleanSubDir(const char *dir);
   int doCleanUp();
   virtual int actualFileSize(int netFileSize) { return netFileSize; }
   void freeSpace();
   bool exists(const char* file);
   
   long byteCount;
   cString currentDir;
private:
   bool failedFreeSpace;
};

class LegacyStorage : public Storage {
private:
   void initMaxStorage(int maxMB=-1);
public:
   LegacyStorage(int maxMB);
   virtual ~LegacyStorage();
   virtual void cleanUp();
   
   virtual StorageHandle openForWriting(PageID page);
   virtual StorageHandle openForReading(PageID page, bool countAsAccess);
   virtual ssize_t write(const void *ptr, size_t size, StorageHandle stream);
   virtual ssize_t read(void *ptr, size_t size, StorageHandle stream)
     { return ::read((int)stream, ptr, size); }
   virtual void close(StorageHandle stream)
     { ::close((int)stream); }
protected:
   void registerFile(PageID page);
   virtual int actualFileSize(int netFileSize);
   //int maxPages;
   long maxBytes;
   int fsBlockSize;
   int pageBytes;
};

class PackedStorage : public LegacyStorage {
public:
   PackedStorage(int maxMB);
   
   virtual void getFilename(char *buffer, int bufLength, PageID page);
   virtual StorageHandle openForWriting(PageID page);
   virtual StorageHandle openForReading(PageID page, bool countAsAccess);
protected:
   struct PageAddress {
      bool operator==(const PageID &id) const
        { return page==id.page && subPage==id.subPage; }
      void operator=(const PageID &id)
        { page=id.page; subPage=id.subPage; }
      int page;
      int subPage;
   };
   bool seekTo(PageID page, int fd, bool create);
   void registerFile(PageID page);
};

class cTelePage {
 private:
  int mag;
  unsigned char flags;
  unsigned char lang;
  PageID page;
  unsigned char pagebuf[27*40];
  Storage* storage;
 public:
  cTelePage(PageID page, uchar flags, uchar lang, int mag, Storage *s);
  ~cTelePage();
  void SetLine(int, uchar*);
  void save();
  bool IsTopTextPage();
 };

class cRingTxtFrames : public cRingBufferFrame {
 public:
  cRingTxtFrames(int Size) : cRingBufferFrame(Size, true) {};
  ~cRingTxtFrames() { Clear(); };
  void Wait(void) { WaitForGet(); };
  void Signal(void) { EnableGet(); };
};

class cTxtReceiver : public cReceiver, public cThread {
private:
   void DecodeTXT(uchar*);
   uchar unham16 (uchar*);
   cTelePage *TxtPage;
   void SaveAndDeleteTxtPage();
   bool storeTopText;
   cRingTxtFrames buffer;
   Storage *storage;
protected:
   virtual void Activate(bool On);
   virtual void Receive(uchar *Data, int Length);
   virtual void Action();
public:
   cTxtReceiver(int TPid, tChannelID chan, bool storeTopText, Storage* storage);
   virtual ~cTxtReceiver();
   virtual void Stop();
};

class cTxtStatus : public cStatus {
private:
   cTxtReceiver *receiver;
   bool storeTopText;
   Storage* storage;
protected:
   virtual void ChannelSwitch(const cDevice *Device, int ChannelNumber);
public:
   cTxtStatus(bool storeTopText, Storage* storage);
   ~cTxtStatus();

#ifdef REELVDR
   void CheckDeleteReceiver();
   void CheckCreateReceiver();
#endif
};


#endif
