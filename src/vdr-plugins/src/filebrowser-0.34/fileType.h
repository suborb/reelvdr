/***************************************************************************
 *   Copyright (C) 2005 by Reel Multimedia                                 *
 *                                                                         *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#ifndef P__FILETYPE_H
#define P__FILETYPE_H

#include <vdr/osdbase.h>
#include <vector>
#include <string>

class cImageConvert;
class cMenuFileBrowserBase;
class cPlayListItem;
class cFileTypeBase;
class cMenuFileItem;

enum efiletype
{
    tp_unknown, tp_jpg, tp_png, tp_gif, tp_tiff, tp_bmp, tp_mpg, tp_avi, tp_mp3, tp_cue, tp_playlist, tp_iso, 
    tp_unspecifiedpicture, tp_unspecifiedvideo, tp_unspecifiedaudio, tp_deb, tp_po
};

#define CL_UNKNOWN      0x00000001
#define CL_PICTURE      0x00000002
#define CL_XINE         0x00000004
#define CL_VIDEO        0x00000008
#define CL_AUDIO        0x00000010
#define CL_DEB          0x00000020
#define CL_PLAYLIST     0x00000040
#define CL_PO		0x00000080

enum efileclass
{
    cl_unknown = CL_UNKNOWN,
    cl_picture = CL_PICTURE,
    cl_xine = CL_XINE,
    cl_video = CL_VIDEO,
    cl_audio = CL_AUDIO,
    cl_deb = CL_DEB,
    cl_playlist = CL_PLAYLIST,
    cl_po = CL_PO
};

//----------cFileType-----------------------

class cFileType 
{
public:
    cFileType(efiletype ftype);
    cFileType(std::string path);
    ~cFileType(){};
    efiletype GetType() const;
    efileclass GetClass() const;
    std::string GetSymbol() const;
    bool IsOfSimilarType(cFileType filetype) const;
    bool IsPlayable() const;
    bool HasInfo() const;
    std::vector<std::string> GetFileInfo(std::string path) const;
    eOSState OpenFile(cPlayListItem &item, cMenuFileBrowserBase *menu, bool asPlaylist = false)  const;
    eOSState UpdateFile(cPlayListItem &item, cMenuFileBrowserBase *menu, bool currentChanched) const;
    bool operator == (cFileType type) const
    {
        return type_ == type.type_;
    }

private:
    void GetFileType(std::string path);
    void GetFileType(efiletype ftype);
    cFileTypeBase *type_;
};

//----------cFileTypeBase-----------------------

class cFileTypeBase 
{
protected:
    cFileTypeBase(efiletype ftype = tp_unknown, efileclass fclass = cl_unknown)
    :ftype_(ftype), fclass_(fclass){};
    static cFileTypeBase instance;
public:
    virtual ~cFileTypeBase(){};
    efiletype GetType() const {return ftype_;}
    efileclass GetClass() const {return fclass_;}
    virtual bool IsOfSimilarType(cFileType filetype) const;
    virtual bool IsPlayable() const {return false;}
    virtual bool HasInfo() const {return false;}
    virtual std::string GetSymbol() const {return std::string();}
    virtual std::vector<std::string> GetFileInfo(std::string path) const {
    printf("------cFileTypeBase, GetFileInfo\n"); return std::vector<std::string>();}
    virtual eOSState OpenFile(cPlayListItem &item, cMenuFileBrowserBase *menu, bool asPlaylist = false) const;
    virtual eOSState UpdateFile(cPlayListItem &item, cMenuFileBrowserBase *menu, bool currentChanched) const;
    static cFileTypeBase &Instance(){return instance;}

protected:
    efiletype ftype_;
    efileclass fclass_;
};

//----------cFileTypeBaseXine-----------------------

class cFileTypeBaseXine : public cFileTypeBase
{
protected:
    cFileTypeBaseXine(efiletype ftype, efileclass fclass = cl_xine)
    :cFileTypeBase(ftype, fclass){};
public:
    /*override*/ ~cFileTypeBaseXine(){}
    /*override*/ bool IsPlayable() const {return true;}
    /*override*/ bool HasInfo() const {return true;}
    /*override*/ eOSState OpenFile(cPlayListItem &item, cMenuFileBrowserBase *menu, bool asPlaylist = false) const;
};

//----------cFileTypeBasePicture-----------------------

class cFileTypeBasePicture : public cFileTypeBase
{
protected:
    cFileTypeBasePicture(efiletype ftype)
    :cFileTypeBase(ftype, cl_picture){}
public:
    /*override*/ ~cFileTypeBasePicture(){}
    /*override*/ bool IsPlayable() const {return true;}
    /*override*/ bool HasInfo() const {return false;}
    /*override*/ eOSState OpenFile(cPlayListItem &item, cMenuFileBrowserBase *menu, bool asPlaylist = false) const;
    /*override*/ eOSState UpdateFile(cPlayListItem &item, cMenuFileBrowserBase *menu, bool currentChanched) const;
};

//----------cFileTypeBaseVideo-----------------------

class cFileTypeBaseVideo : public cFileTypeBaseXine
{
protected:
    cFileTypeBaseVideo(efiletype ftype)
    :cFileTypeBaseXine(ftype, cl_video){}
public:
    /*override*/ ~cFileTypeBaseVideo(){}
};

//----------cFileTypeBaseAudio-----------------------

class cFileTypeBaseAudio : public  cFileTypeBaseXine
{
protected:
    cFileTypeBaseAudio(efiletype ftype)
    :cFileTypeBaseXine(ftype, cl_audio){}
public:
    /*override*/ ~cFileTypeBaseAudio(){}
    /*override*/ std::string GetSymbol() const;
};

//----------cFileTypeBaseUnspecifiedPicture------------

class cFileTypeBaseUnspecifiedPicture : public cFileTypeBasePicture
{ 
    cFileTypeBaseUnspecifiedPicture()
    :cFileTypeBasePicture(tp_unspecifiedpicture){}
    static cFileTypeBaseUnspecifiedPicture instance;
public:
    static cFileTypeBaseUnspecifiedPicture &Instance(){return instance;}
    /*override*/ ~cFileTypeBaseUnspecifiedPicture(){}
    static bool IsUnspecifiedPicture(std::string path);
};

//----------cFileTypeBasePNG-----------------------

class cFileTypeBasePNG : public cFileTypeBasePicture
{ 
    cFileTypeBasePNG()
    :cFileTypeBasePicture(tp_png){}
    static cFileTypeBasePNG instance;
public:
    static cFileTypeBasePNG &Instance(){return instance;}
    /*override*/ ~cFileTypeBasePNG(){}
    static bool IsPNG(std::string path);
};

//----------cFileTypeBaseTIFF-----------------------

class cFileTypeBaseTIFF : public cFileTypeBasePicture
{
    cFileTypeBaseTIFF()
    :cFileTypeBasePicture(tp_tiff){}
    static cFileTypeBaseTIFF instance;
public:
    static cFileTypeBaseTIFF &Instance(){return instance;}
    /*override*/ ~cFileTypeBaseTIFF(){}
    static bool IsTIFF(std::string path);
};

//----------cFileTypeBaseBMP-----------------------

class cFileTypeBaseBMP : public cFileTypeBasePicture
{
    cFileTypeBaseBMP()
    :cFileTypeBasePicture(tp_bmp){}
    static cFileTypeBaseBMP instance;
public:
    static cFileTypeBaseBMP &Instance(){return instance;}
    /*override*/ ~cFileTypeBaseBMP(){}
    static bool IsBMP(std::string path);
};

//----------cFileTypeGIF-----------------------

class cFileTypeBaseGIF : public cFileTypeBasePicture
{
    cFileTypeBaseGIF()
    :cFileTypeBasePicture(tp_gif){}
    static cFileTypeBaseGIF instance;
    public:
    static cFileTypeBaseGIF &Instance(){return instance;}
    /*override*/ ~cFileTypeBaseGIF(){}
    static bool IsGIF(std::string path);
};

//----------cFileTypeBaseJPG-----------------------

class cFileTypeBaseJPG : public cFileTypeBasePicture
{
    cFileTypeBaseJPG()
    :cFileTypeBasePicture(tp_jpg){}
    static cFileTypeBaseJPG instance;
    public:
    static cFileTypeBaseJPG &Instance(){return instance;}
    /*override*/ bool IsPlayable() const {return true;}
    static bool IsJPG(std::string path);
};

//----------cFileTypeBaseUnspecifiedVideo------------

class cFileTypeBaseUnspecifiedVideo : public cFileTypeBaseVideo
{ 
    cFileTypeBaseUnspecifiedVideo()
    :cFileTypeBaseVideo(tp_png){}
    static cFileTypeBaseUnspecifiedVideo instance;
public:
    static cFileTypeBaseUnspecifiedVideo &Instance(){return instance;}
    /*override*/ ~cFileTypeBaseUnspecifiedVideo(){}
    static bool IsUnspecifiedVideo(std::string path);
};

//----------cFileTypeBaseCUE-----------------------
class cFileTypeBaseCUE : public  cFileTypeBaseVideo
{
    cFileTypeBaseCUE()
    :cFileTypeBaseVideo(tp_cue){}
    static cFileTypeBaseCUE instance;
public:
    static cFileTypeBaseCUE &Instance(){return instance;}
    /*override*/ ~cFileTypeBaseCUE(){}
    static bool IsCUE(std::string path);
};

//----------cFileTypeBaseMPG-----------------------
class cFileTypeBaseMPG : public cFileTypeBaseVideo
{
    cFileTypeBaseMPG()
    :cFileTypeBaseVideo(tp_mpg){}
    static cFileTypeBaseMPG instance;
public:
    static cFileTypeBaseMPG &Instance(){return instance;}
    /*override*/ ~cFileTypeBaseMPG(){}
    static bool IsMPG(std::string path);
};

//----------cFileTypeBaseAVI-----------------------
class cFileTypeBaseAVI : public  cFileTypeBaseVideo
{
    cFileTypeBaseAVI()
    :cFileTypeBaseVideo(tp_avi){}
    static cFileTypeBaseAVI instance;
public:
    static cFileTypeBaseAVI &Instance(){return instance;}
    /*override*/ ~cFileTypeBaseAVI(){};  
    static bool IsAVI(std::string path);
};

//----------cFileTypeBaseUnspecifiedAudio------------

class cFileTypeBaseUnspecifiedAudio : public cFileTypeBaseAudio
{ 
    cFileTypeBaseUnspecifiedAudio()
    :cFileTypeBaseAudio(tp_png){}
    static cFileTypeBaseUnspecifiedAudio instance;
public:
    static cFileTypeBaseUnspecifiedAudio &Instance(){return instance;}
    /*override*/ ~cFileTypeBaseUnspecifiedAudio(){}
    static bool IsUnspecifiedAudio(std::string path);
};

//----------cFileTypeBaseMP3-----------------------
class cFileTypeBaseMP3 : public  cFileTypeBaseAudio
{
    cFileTypeBaseMP3()
    :cFileTypeBaseAudio(tp_mp3){}
    static cFileTypeBaseMP3 instance;
public:
    static cFileTypeBaseMP3 &Instance(){return instance;}
    /*override*/ ~cFileTypeBaseMP3(){}
    /*override*/ std::vector<std::string> GetFileInfo(std::string path) const;
    /*override*/ eOSState UpdateFile(cPlayListItem &item, cMenuFileBrowserBase *menu, bool currentChanched) const;
    static bool IsMP3(std::string path);
};

//----------cFileTypeBasePlaylist-----------------------
class cFileTypeBasePlaylist : public  cFileTypeBase
{
    cFileTypeBasePlaylist()
    :cFileTypeBase(tp_playlist, cl_playlist){};
    static cFileTypeBasePlaylist instance;
public:
    static cFileTypeBasePlaylist &Instance(){return instance;}
    /*override*/ ~cFileTypeBasePlaylist(){}
    /*override*/ bool IsPlayable() const {return false;}
    /*override*/ bool HasInfo() const {return false;}
    /*override*/ eOSState OpenFile(cPlayListItem &item, cMenuFileBrowserBase *menu, bool asPlaylist = false) const;
    static bool IsPlaylist(std::string path);
};

//----------cFileTypeBaseISO-----------------------
class cFileTypeBaseISO : public  cFileTypeBase
{
    cFileTypeBaseISO()
    :cFileTypeBase(tp_iso, cl_playlist){};
    static cFileTypeBaseISO instance;
public:
    static cFileTypeBaseISO &Instance(){return instance;}
    /*override*/ ~cFileTypeBaseISO(){}
    /*override*/ bool IsPlayable() const {return true;}
    /*override*/ std::string GetSymbol() const;
    /*override*/ eOSState OpenFile(cPlayListItem &item, cMenuFileBrowserBase *menu, bool asPlaylist = false) const;
    static bool IsISO(std::string path);
};

//----------cFileTypeBaseDebPacket-----------------------
class cFileTypeBaseDebPacket : public  cFileTypeBase
{
    cFileTypeBaseDebPacket()
    :cFileTypeBase(tp_deb, cl_deb){};
    static cFileTypeBaseDebPacket instance;
public:
    static cFileTypeBaseDebPacket &Instance(){return instance;}
    /*override*/ ~cFileTypeBaseDebPacket(){}
    /*override*/ bool IsPlayable() const {return false;}
    /*override*/ bool HasInfo() const {return false;}
    /*override*/ eOSState OpenFile(cPlayListItem &item, cMenuFileBrowserBase *menu, bool asPlaylist = false) const;
    static bool IsDebPacket(std::string path);
};

//----------cFileTypeBasePoFile--------------------------
class cFileTypeBasePoFile : public  cFileTypeBase
{
    cFileTypeBasePoFile()
    :cFileTypeBase(tp_po, cl_po){};
    static cFileTypeBasePoFile instance;
public:
    static cFileTypeBasePoFile &Instance(){return instance;}
    /*override*/ ~cFileTypeBasePoFile(){}
    /*override*/ bool IsPlayable() const {return false;}
    /*override*/ bool HasInfo() const {return false;}
    /*override*/ eOSState OpenFile(cPlayListItem &item, cMenuFileBrowserBase *menu, bool asPlaylist = false) const;
    static bool IsPoFile(std::string path);
};

#endif //P__FILETYPE_H
