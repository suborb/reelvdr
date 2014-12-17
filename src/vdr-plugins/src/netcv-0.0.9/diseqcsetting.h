/***************************************************************************
 *   Copyright (C) 2007 by Reel Multimedia;  Author:  Florian Erfurth      *
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
 *                                                                         *
 ***************************************************************************
 *
 * diseqcsetting.h
 *
 ***************************************************************************/

#ifndef DISEQCSETTING_H
#define DISEQCSETTING_H

#include <libxml/tree.h>
#include <vdr/osdbase.h>
#include <vdr/diseqc.h>
#include <vdr/menu.h>
#include <mcliheaders.h>
#include "netcvtuner.h"
#include "netcvcam.h"

#define SATLIST_STRING_LENGTH 16

#if APIVERSNUM >= 10716
#define DISEQC_SOURCE_ASTRA_19          cSource::FromString("S19.2E")
#define DISEQC_SOURCE_HOTBIRD_13        cSource::FromString("S13.0E")
#define DISEQC_SOURCE_ASTRA_28          cSource::FromString("S28.2E")
#define DISEQC_SOURCE_ASTRA_23          cSource::FromString("S23.5E")
#define DISEQC_SOURCE_TURKSAT_42        cSource::FromString("S42.0E")
#define DISEQC_SOURCE_HISPASAT_30W      cSource::FromString("S30.0W")
#define DISEQC_SOURCE_INTELSAT_1W       cSource::FromString("S1.0W")
#else
#define DISEQC_SOURCE_ASTRA_19 35008
#define DISEQC_SOURCE_HOTBIRD_13 34946
#define DISEQC_SOURCE_ASTRA_28 35098
#define DISEQC_SOURCE_ASTRA_23 35051
#define DISEQC_SOURCE_TURKSAT_42 35236
#define DISEQC_SOURCE_HISPASAT_30W 33068
#define DISEQC_SOURCE_INTELSAT_1W 32778
#endif /*APIVERSNUM >= 10716*/

enum eDiseqcMode { eDiseqcDisabled, eDiseqcMini, eDiseqc1_0, eDiseqc1_1,
                   eDiseqc1_2, eGotoX, eDisiCon4, eSingleCableRouter };
extern const char* AutoFocusStrings[];

typedef struct {
    xmlDocPtr doc;
    xmlChar *str, *key;
} xml_parser_context_t;

#define DISEQC_MAX_EXTRA 8
#define UUID_SIZE 256

typedef struct
{
    int PositionMin;
    int PositionMax;
    int Longitude;
    int Latitude;
    int AutoFocus;
} rotor_info_t;

class SingleCableRouter_s
{
public:
    SingleCableRouter_s(): uniqSlotNr(0), downlinkFreq(900), pin(-1) {
    }

    // Slot number 0..7
    int uniqSlotNr;

    // downlink frequency 900-2200MHz, 4 or 8 possibilites
    int downlinkFreq;

    // PIN  0..255; No PIN set indicated by < 0 values
    int pin;



    bool get_from_xx_yy(int xx, int yy) {
        /* ensure each param is one byte */
        xx = 0xff & xx;
        yy = 0xff & yy;

        /* last 2 bits of xx and yy give Z()*/
        int half_freq = (((xx&0x03)<<8) | yy) + 350;
        downlinkFreq = 2*half_freq;

        /*3 msb of xx*/
        uniqSlotNr = ((xx>>5)&0x07);
        return true;
    }

    bool IsValidFreq() const {
        return downlinkFreq >= 900 && downlinkFreq <= 2200;
    } // IsValidFreq()


    /** helpers to compute diseq commands
     *  Diseqc command: e0 10 5a XX YY [PIN]
     */
    int Z() const {
        return downlinkFreq/2 - 350;
    } // Z()

    int YY() const {
        return Z()&0xff; // 8 lsb of Z()
    } // YY()

    int PIN() const {

        if (pin<0) return -1; // no pin set

        return pin&0xff;
    } //PIN()


    int XX(int isSat, int pol /*V=0, H=1*/, int band /*high=1, low=0*/) const {
        int xx;
        xx = ((uniqSlotNr&0x07)<<5) /*7-5 bits*/
                | ((isSat&0x01)<<4) /*bit 4*/
                | ((pol&0x01)<<3)   /*bit 3*/
                | ((band&0x01)<<2)  /*bit 2*/
                | ((Z()>>8)&0x03);  /*bit 1-0*/

        return xx & 0xff;
    } // XX()


    /**  Diseqc command: e0 10 5a XX YY [PIN] */
    std::string DiseqcCommand(bool isSat, bool isHorizPol, bool isBandHigh) const {
        char buff[32];
        int xx = XX(isSat, isHorizPol, isBandHigh);
        int yy = YY();

        if (pin<0)
            snprintf(buff, 31, "e0 10 5a %x %x", xx, yy);
        else /* valid pin*/
            snprintf(buff, 31, "e0 10 5a %x %x %x", xx, yy, PIN());

        return buff;
    } // DiseqcCommand()

};

typedef SingleCableRouter_s SingleCableRouter_t;


class cDiseqcSettingBase;
//class cDiseqcSetting;

class cLNBSetting
{
    public:
        int iSource_;
        int iLNB_Type_;

        cLNBSetting();
        ~cLNBSetting();

        std::string GetSatName() { return std::string(*cSource::ToString(iSource_)); };
        std::string GetSatPosition();
        int GetSource() { return iSource_; };
        int SetSource(const char *SatName);
        int GetLNBType() { return iLNB_Type_; };
        int SetLNBType(satellite_info_t *sat);
        std::string GetRangeMin(bool HighLOF);
        std::string GetRangeMax(bool HighLOF);
        std::string GetLOF(bool HighLOF);
};

class cLNBSettingDiseqc1_1 : public cLNBSetting
{
    public:
        int iMultiswitch_; // 0=false 1=true
        int iCascade_;

        cLNBSettingDiseqc1_1();
        ~cLNBSettingDiseqc1_1();
};

class cSatList
{
    private:
        std::vector<cDiseqcSettingBase*> DiseqcMode_;
        std::vector<cDiseqcSettingBase*>::iterator modeIter_;

    public:
        char *Name_;
        bool bShowSettings_;

        cSatList(const char* name = NULL);
        ~cSatList();

        void BuildMenu(cOsdMenu *menu);
        int CheckSetting();
        int SetSettings(eDiseqcMode DiseqcMode, satellite_list_t *sat_list);
        int SetMode(eDiseqcMode DiseqcMode);
        eDiseqcMode GetMode();
        std::vector<cDiseqcSettingBase*>::iterator GetModeIterator() { return modeIter_; };
};

class cNetCVTuner;
class cNetCVTuners;
class cDiseqcSetting
{
    private:
        int AddCamComponent(xmlNodePtr node, NetCVCam *Cam);
        int AddTunerComponent(xmlNodePtr node, cNetCVTuner *Tuner, int Slot);
        int AddSatComponent(xmlNodePtr node, cDiseqcSettingBase *Diseqcsetting);
        int Upload(const char* FileName);
        int Download();
        void Restart(bool bRestartVDR);
        int ReadNetceiverConf(const char *filename, netceiver_info_t *nc_info);
        int CheckSetting();
        int WriteSetting();

        std::vector<cSatList*> SatList_;
        const char *TmpPath_;
        const char *Interface_;
        const char *NetceiverUUID_;
        cNetCVCams *Cams_;
        cNetCVTuners *Tuners_;

    public:
        cDiseqcSetting(const char *TmpPath, const char *Interface, const char *NetceiverUUID, cNetCVCams *Cams, cNetCVTuners *Tuners);
        ~cDiseqcSetting();

        int LoadSetting();
        int SaveSetting();
        int SetSettings(netceiver_info_t *nc_info);
        eDiseqcMode DetectDiseqcMode(satellite_list_t *sat_list);
        void BuildMenu(cOsdMenu *menu);
        std::string GenerateSatlistName();
        void AddSatlist(const char* name = NULL) { SatList_.push_back(new cSatList(name)); };
        std::vector<cSatList*>* GetSatlistVector() { return &SatList_; };
        cSatList* GetSatlistByName(const char* Name);
};

class cDiseqcSettingBase
{
    public:
        const char *name_;
        const eDiseqcMode mode_;

        cDiseqcSettingBase(const char *Name, const eDiseqcMode Mode);
        virtual ~cDiseqcSettingBase() {};

        void AddMode(cOsdMenu *menu, std::vector<cDiseqcSettingBase*>::iterator *modeIter, std::vector<cDiseqcSettingBase*>::iterator firstIter, std::vector<cDiseqcSettingBase*>::iterator lastIter);
        virtual int SetSettings(satellite_list_t *sat_list) = 0;
        virtual void BuildMenu(cOsdMenu *menu) = 0;
        virtual int GetNumberOfLNB() = 0;
        virtual void SetNumberOfLNB(int number) = 0;
        virtual cLNBSetting* GetLNBSetting(int index=0) = 0;
};

class cDiseqcDisabled : public cDiseqcSettingBase
{
    public:
        cLNBSetting *LNBSetting_;

        cDiseqcDisabled();
        ~cDiseqcDisabled();

        /*override*/ int SetSettings(satellite_list_t *sat_list) = 0; // TODO: Still not implemented
        /*override*/ void BuildMenu(cOsdMenu *menu);
        /*override*/ int GetNumberOfLNB() { return 1; };
        /*override*/ void SetNumberOfLNB(int number) {};
        /*override*/ cLNBSetting* GetLNBSetting(int index) { return LNBSetting_; };
};

class cDiseqcMini : public cDiseqcSettingBase
{
    private:
        const int iMaxLNB_;
        int iNumberOfLNB_;

    public:
        std::vector<cLNBSetting*> LNBSetting_;
//        int iDelay_;

        cDiseqcMini();
        ~cDiseqcMini();

        /*override*/ int SetSettings(satellite_list_t *sat_list) = 0; // TODO: Still not implemented
        /*override*/ void BuildMenu(cOsdMenu *menu);
        /*override*/ int GetNumberOfLNB() { return iNumberOfLNB_; };
        /*override*/ void SetNumberOfLNB(int number);
        /*override*/ cLNBSetting* GetLNBSetting(int index) { return LNBSetting_.at(index); };
};

class cDiseqc1_0 : public cDiseqcSettingBase
{
    private:
        const int iMaxLNB_;
        int iNumberOfLNB_;

    public:
        std::vector<cLNBSetting*> LNBSetting_;
//        int iToneburst_;
//        int iDelay_;
//        int iRepeat_;

        cDiseqc1_0();
        ~cDiseqc1_0();

        /*override*/ int SetSettings(satellite_list_t *sat_list);
        /*override*/ void BuildMenu(cOsdMenu *menu);
        /*override*/ int GetNumberOfLNB() { return iNumberOfLNB_; };
        /*override*/ void SetNumberOfLNB(int number);
        /*override*/ cLNBSetting* GetLNBSetting(int index) { return LNBSetting_.at(index); };
};

class SingleCableRouter_s;

class cDiseqcSCR : public cDiseqcSettingBase
{
    private:
        const int iMaxLNB_;
        int iNumberOfLNB_;

    public:
        std::vector<cLNBSetting*> LNBSetting_;
        SingleCableRouter_s scrProps;

//        int iToneburst_;
//        int iDelay_;
//        int iRepeat_;

        cDiseqcSCR();
        ~cDiseqcSCR();

        /*override*/ int SetSettings(satellite_list_t *sat_list);
        /*override*/ void BuildMenu(cOsdMenu *menu);
        /*override*/ int GetNumberOfLNB() { return iNumberOfLNB_; };
        /*override*/ void SetNumberOfLNB(int number);
        /*override*/ cLNBSetting* GetLNBSetting(int index) { return LNBSetting_.at(index); };
};


class cDiseqc1_1 : public cDiseqcSettingBase
{
    private:
        int iMaxLNB_; // not const because it depends on existence of multiswitch
        int iNumberOfLNB_;

    public:
        std::vector<cLNBSettingDiseqc1_1*> LNBSettingDiseqc1_1_;
        int iMultiswitch_; // 0=no; 1=yes;
//        int iToneburst_;
//        int iDelay_;
//        int iRepeat_;

        cDiseqc1_1();
        ~cDiseqc1_1();

        /*override*/ int SetSettings(satellite_list_t *sat_list);
        /*override*/ void BuildMenu(cOsdMenu *menu);
        /*override*/ int GetNumberOfLNB() { return iNumberOfLNB_; };
        /*override*/ void SetNumberOfLNB(int number);
        /*override*/ cLNBSettingDiseqc1_1* GetLNBSetting(int index) { return LNBSettingDiseqc1_1_.at(index); };
};

class cDiseqc1_2 : public cDiseqcSettingBase
{
    public:
        int iLNB_Type_;
        const int iMaxSource_;
        int iNumberOfSource_;
        std::vector<int> iSource_;

        cDiseqc1_2();
        ~cDiseqc1_2();

        /*override*/ int SetSettings(satellite_list_t *sat_list) = 0; // TODO: Still not implemented
        /*override*/ void BuildMenu(cOsdMenu *menu);
        /*override*/ int GetNumberOfLNB() { return 0; };
        /*override*/ void SetNumberOfLNB(int number) {};
        /*override*/ cLNBSetting* GetLNBSetting(int index) { return NULL; };
};

class cGotoX : public cDiseqcSettingBase
{
    public:
        int iPositionMin_;
        int iPositionMax_;
        int iLongitude_;
        int iLatitude_;
        int iAutoFocus_;

        cGotoX();
        ~cGotoX();

        /*override*/ int SetSettings(satellite_list_t *sat_list);
        /*override*/ void BuildMenu(cOsdMenu *menu);
        /*override*/ int GetNumberOfLNB() { return 0; };
        /*override*/ void SetNumberOfLNB(int number) {};
        /*override*/ cLNBSetting* GetLNBSetting(int index) { return NULL; };
};

class cDisicon4 : public cDiseqcSettingBase
{
    public:
        cLNBSetting *LNBSetting_;
        int iSource_;

        cDisicon4();
        ~cDisicon4();

        /*override*/ int SetSettings(satellite_list_t *sat_list) = 0; // TODO: Still not implemented
        /*override*/ void BuildMenu(cOsdMenu *menu);
        /*override*/ int GetNumberOfLNB() { return 0; };
        /*override*/ void SetNumberOfLNB(int number) {};
        /*override*/ cLNBSetting* GetLNBSetting(int index) { return LNBSetting_; };
};

#endif
