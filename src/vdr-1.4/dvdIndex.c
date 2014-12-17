#include <string.h> 			//strdup()
#include <stdio.h>  			//popen ()
#include "dvdIndex.h"

/// computes sha1 sum of a directory by
// listing all files of the directory and their respective sizes

/**
 *  cd _mount_point_
 *  ls -Rl | awk '{ if ( NF >= 8 ) printf "%s %s ", $5,  $8}' ==> get the size ($5) and the name ($8) of all files in _mount_point_
 *  sha1sum ==> compute SHA1 sum
 *  ... awk to get the first column which is the sha1sum
 */

std::string SHA_directory(std::string dir)
{
	char cmd [512];
	char* sha1_str=NULL;

	sprintf( cmd, "cd \"%s\" ; ls -Rl |\
			awk '{ if ( NF >= 8 ) printf \"%%s %%s \", $5,  $8}' |\
			sha1sum | awk '{print $1}' ", 
			dir.c_str() );

	FILE *p = popen(cmd, "r"); // code from devicesetup.c -  device::mount()
	if (p) {
		char s[64];
		fscanf(p," %s", s);
		sha1_str = strdup(s);
	}
	pclose(p);

	return sha1_str; // len(sha1_str) != 40 => ERROR

}


/// ---- class cDataBaseEntry -------------
// one entry of DataBase
cDataBaseEntry::cDataBaseEntry()		 { }
cDataBaseEntry::cDataBaseEntry( string plName, string volName, long long mSize, long long t )
{ 
	pluginName = plName; 
	volumeName = volName;
	lastMounted = t?t:time(0);  // present time
	mediaSize = mSize;
}

time_t 		cDataBaseEntry::LastMounted() 	{ return lastMounted; }
long long 	cDataBaseEntry::MediaSize() 	{ return mediaSize; }
string 		cDataBaseEntry::VolName() 		{ return volumeName; }
string 		cDataBaseEntry::PluginName() 	{ return pluginName; }


///---- class CMediaDataBase ----------

cMediaDataBase::cMediaDataBase(string fileName) 
{ 
	dbFile = fileName; 
	mediaDB.clear(); 
	Read();
	dbIter=mediaDB.begin();
}       ///

bool cMediaDataBase::Next(cDataBaseEntry& node) // get current entry, and increament dbIter
{
	if ( mediaDB.size() <= 0 || dbIter == mediaDB.end() ) return false;

	node = dbIter->second;
	dbIter++;
	return true;
}

void cMediaDataBase::Rewind() // goto the first entry in DB
{
	dbIter = mediaDB.begin();
}


bool cMediaDataBase::Save()            /// writes mediaDB object to the DataBase in HDD
{
	FILE *fp = fopen(dbFile.c_str(),"w+");
	if (!fp) return false;

	int flag= -1; // so if for-loop is skipped => nothing written to HDD => nothing saved .ie return false
	map<string, cDataBaseEntry>::iterator it = mediaDB.begin();

	for ( ; it != mediaDB.end() ; it++)
	{
		flag = fprintf(fp, "%s\t%s\t%s\t%lld\t%lld\n", 
				it->first.c_str(), it->second.PluginName().c_str() , 
				it->second.VolName().c_str(), it->second.MediaSize(),
				(long long)it->second.LastMounted() );

		if (flag<0) // error
		{ 
			printf("(%s:%d) flag < 0: not saved\n",__FILE__,__LINE__);
			break;
		}
	}

	fclose(fp);
	return (flag >= 0);

}


bool cMediaDataBase::Read()            /// reads from HDD, and populates mediaDB object 
{

	if ( access( dbFile.c_str(), F_OK ) != 0 ) // file does not exist
	{
		printf("(%s:%d) db doesnot exist!! \n", __FILE__, __LINE__);
		mediaDB.clear();
		return true;
	}
	FILE *fp = fopen(dbFile.c_str(), "r");
	if ( !fp ) // read error!
	{
             //PRINTF("(%s:%d) read error: %s \n", __FILE__, __LINE__, dbFile.c_str());
             return false; // mediaDB not modified
	}

	// tmp DB
	map<string, cDataBaseEntry> tmpDB;
	string plName, volName, key;
	long long t, s;
	char buffer[512];
	int flag = -1;

	while( !feof(fp) )
	{
		flag = fscanf(fp, " %s",buffer); // key
		key = buffer;
		if (flag<0) break;

		flag = fscanf(fp, " %s",buffer); // pluginName
		plName = buffer;
		if (flag<0) break;

		flag = fscanf(fp, " %s",buffer); // VolName
		volName = buffer;
		if (flag<0) break;

		flag = fscanf(fp, " %lld", &s); // size
		if (flag<0) break;
		flag = fscanf(fp, " %lld", &t); // time
		if (flag<0) break;
		/// TODO replace with a single fscanf()


		cDataBaseEntry node (plName, volName, s, t);
		tmpDB[key] = node;

	} // end while

	if ( flag<0 && ferror(fp) ) // flag alone might indicate EOF
	{ fclose(fp); printf("flag=%d\n",flag); return false; }

	fclose(fp);
	mediaDB.clear();
	mediaDB.insert( tmpDB.begin(), tmpDB.end() );
	return true;
} //end Read()

bool cMediaDataBase::Add(string hashKey, cDataBaseEntry node)
{
	mediaDB[hashKey] = node ;
	return Save();
}

/// add new entry into DataBase: calls Save() after updating mediaDB object
bool cMediaDataBase::Add(string hashKey, string plName, string volName, long long s) 
{
	cDataBaseEntry node(plName, volName, s);
	return Add(hashKey, node);
}

void cMediaDataBase::Remove(string hashKey) /// remove entry and write it to HDD
{ 
	mediaDB.erase(hashKey); 
	Save(); 
}  

long cMediaDataBase::Size()  /// number of entries
{ 
	return mediaDB.size(); 
}           

void cMediaDataBase::Clear()  /// clears DataBase
{ 
	mediaDB.clear(); 
	Save(); 
}        

/// end class  cMediaDataBase
