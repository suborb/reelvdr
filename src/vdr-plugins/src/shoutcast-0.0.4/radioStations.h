#ifndef _RADIOSTATION__H_
#define _RADIOSTATION__H_

#include<string>
#include<vector>

using namespace std;

class cServerInfo
{
    public:
        cServerInfo(const string name, const string url);
        cServerInfo();

        string Name;
        string Url;
        bool operator < (const cServerInfo& a) const { return this->Name < a.Name; }
};
////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////


/**
 * A singleton class: Read from a file, write to a file the list of User defined radio stations
 *                    also checks for duplicates of station URL/name
 */
class cRadioStation
{
    public:
        ~cRadioStation();
        static cRadioStation *Instance();
        
        bool IsUniqueUrl(std::string url) const;
        bool IsUniqueName(std::string name) const;

        vector<cServerInfo> StationList() const;
        bool Save();
        void ForgetChanges();

        bool AddStation(string name, string url);
        bool ReplaceStation( int index, const cServerInfo newServerInfo);

        bool DelStation(string name, string url);
        bool DelStation(unsigned int);

        bool IsEmpty() const;
        bool Dirty() const;
        void ResetDirty();

    private:
        cRadioStation();
        cRadioStation(const cRadioStation&); // no copying 

        void SetDirty();
        //vector<cServerInfo>      ReadStationList();
        bool      ReadStationList();
        vector<cServerInfo>      stationList_;

        static cRadioStation     *instance_;
        bool dirty_;

};
////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
#endif

