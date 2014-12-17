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
 
#ifndef P__CONVERT_H
#define P__CONVERT_H

#include <string>
#include <list>
#include <set>
#include <vector>

#include <vdr/thread.h>

class cFileCache;

enum cvStatus { stNone, stRun, stCopying, stFin};

struct InListElement
{
    std::string item;
    bool live;
    bool operator==(const InListElement &elem)
    {
        return (item == elem.item);
    } 
}; 

struct OutListElement
{
    std::string item;
    bool valid;
    std::vector<unsigned char> buffer; 
}; 

//-----------cImageConvert-------------

class cImageConvert : public cThread 
{
public:
    cImageConvert(cFileCache *cache);
    ~cImageConvert();
    void InsertInConversionList(std::string item, bool live);
    void InsertInConversionList(const std::set<std::string> &newfiles);
    void RemoveFromConversionList(std::string item, bool lock = true);
    bool GetConvertedFile(std::string item, OutListElement& elem);
    void ClearConvertedFiles();
    void StartConversion();
    void WaitUntilCopied();
    bool IsInConversion(std::string item);
    void CancelConversion();

protected:
    
    /*override*/ void Action();    
    virtual int CallConversionScript(std::string img, std::string type, std::string resImg,
            std::string width, std::string height, std::string aspect);
    cFileCache *cache_;
    cCondWait cond;
    std::string currentFile_;
    std::list<OutListElement> outFiles_;
    std::list<InListElement> inFiles_;
    std::vector<unsigned char> buffer_;
    volatile cvStatus status_;
    bool live_;

    std::string GetFileTypeString(std::string item);
    void PutInOutputList(bool success);
    bool GetFromInputList();
    void GetScreenParams(std::string &widthStr, std::string &heightStr, std::string &aspectStr);
    int ReadFileInBuffer(std::string img);
    int CopyFile(std::string destFile, std::string srcFile);
};

//-----------cThumbConvert-------------

class cThumbConvert : public cImageConvert 
{
public:
    cThumbConvert(cFileCache *cache);
    ~cThumbConvert(){};
    void SetDimensions(int width, int height);
    void SetDimensions(int width, int height, int aspect);
    void SetEnding(std::string ending);
    std::string GetEnding() const;

protected:
     /*override*/ void Action();
     /*override*/ int CallConversionScript(std::string img, std::string type, std::string resImg,
           std::string width, std::string height, std::string aspect);

private:
    int width_;
    int height_;
    int aspect_;
    std::string ending_;
};

//----inline-----

inline cImageConvert::cImageConvert(cFileCache *cache) 
: cache_(cache), status_(stNone), live_(false)
{
}

#endif //P__CONVERT_H



