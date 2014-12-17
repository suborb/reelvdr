#ifndef __DVDINDEX_H__
#define __DVDINDEX_H__


#include <map>
#include <string>
#include <time.h>

#define DVD_DB_FILENAME "/etc/vdr/DVD.db"
// TODO: replace with ConfigDirectory/DVD.db

using namespace std;

std::string SHA_directory(std::string);

/// one entry in the Database
class cDataBaseEntry
{
	private:
		string pluginName;       // which of the plugins of mediad accessed the medium
		string volumeName;       //  
		time_t lastMounted;      // last-time when the media was inserted. automatically set in constructor
		long long mediaSize;
	public:
		cDataBaseEntry(); 
		cDataBaseEntry( string plName, string volName, long long mSize, long long t=0 );
		time_t LastMounted(); 
		long long MediaSize();
		string VolName();
		string PluginName();
};


/// The DataBase: needs a unique "key" : now using SHA1
class cMediaDataBase
{
	private:
	/// DB in data structure
	map<string, cDataBaseEntry> mediaDB;  
	map<string, cDataBaseEntry>::iterator dbIter;  /// DB iterator
	/// the database file
	string dbFile; 
	public:
	cMediaDataBase(string fileName);

	// get current entry, and increament dbIter
	bool Next(cDataBaseEntry& node); 
	void Rewind(); // goto the first entry in DB

	bool Save();            /// writes mediaDB object to the DataBase in HDD
	bool Read();            /// reads from HDD, and populates mediaDB object 

	/// add new entry into DataBase: calls Save() after updating mediaDB object
	bool Add(string hashKey, cDataBaseEntry node);
	bool Add(string hashKey, string plName, string volName, long long s); // XXX: remove this function
	void Remove(string hashKey); /// remove entry and write it to HDD

	long Size();  /// number of entries
	void Clear();  /// clears DataBase and writes to DB
};


#endif
