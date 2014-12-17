#include "searchapi.h"
#include "databasereader.h"
#include "createdatabaseinstance.h"
#include <sstream>
#include "sqlutils.h"

unsigned long Atol(const std::string& str)
{
    if (str.empty())
        return 0;

    unsigned long l;
    std::stringstream s(str);
    s >> l;
    return l;
}

cSearchDatabase::cSearchDatabase(std::string &dbPath)
{
    db = CreateDatabaseInstance(dbPath);
    reader = new cDatabaseReader(db);
}

cSearchDatabase::~cSearchDatabase()
{
    // first delete 'reader' and then the database instance
    delete reader;
    delete db;
}

std::string cSearchDatabase::toSqlWildCardString(const std::string& str)
{
    /*
    std::string::size_type idx = str.find('*');
    
    // return empty string if no '*' is present
    if (idx == std::string::npos)
        return std::string();
    */
    std::string result = EscapeSingleQuotes(str);

    //replace all '*' to '%'
    for (unsigned int i=0; i<result.size(); ++i)
        if (result[i]=='*') 
            result[i] = '%';
    
    return result;
}
std::string  cSearchDatabase::generateWhereClause(std::string column, std::string str)
{
    std::string where;
    if (!str.empty()) 
    {
        std::string wildCardStr = toSqlWildCardString(str);
        
        // no wild card 
        /*if (wildCardStr.empty())
            where = column + std::string("=\'") + str + std::string("\'");  // column="str"
        else*/
            where = column + std::string(" like \'") + wildCardStr + std::string("\'"); // column like "str%"
    }
    
    return where;
}

cFileInfo cSearchDatabase::FileInfo(const std::string &path)
{
    cFileInfo file;
    if (path.empty())
        return file;

    cQuery q;
    q.AddField("path");
    q.AddField("filename");
    q.AddField("length");
    q.AddField("title");
    q.AddField("album");
    q.AddField("artist");
    q.AddField("genre");

    std::string where = generateWhereClause("path", path);
    q.AddWhereClause(where);

    cResults result = reader->Query("metadata", q);
    std::vector<cRow>::const_iterator it = result.results.begin();
    for ( ; it != result.results.end(); ++it)
    {
        if (it->size() != 7 /*number of 'AddField()' */)
            continue; // error

        /* in the same order as AddField() */
        file.path       = it->at(0);
        file.filename   = it->at(1);
        file.duration   = Atol(it->at(2));

        file.title      = it->at(3);
        file.album      = it->at(4);
        file.artist     = it->at(5);
        file.genre      = it->at(6);
    }

    return file;
}

/**
 * @brief cSearchDatabase::Files
 * @param genre
 * @param artist
 * @param album
 * @param title
 * @return
 *
 *    std::string title;
    std::string artist;
    std::string genre;
    std::string album;

    std::string path;  // absolute path
    std::string filename; // name of the file

    unsigned long duration;
 */

std::vector<cFileInfo> cSearchDatabase::Files(const std::string& genre, const std::string& artist, const std::string& album, const std::string& title)
{

    if (!reader)
        return std::vector<cFileInfo>();

    cQuery q;
    q.AddField("path");
    q.AddField("filename");
    q.AddField("length");
    q.AddField("title");
    q.AddField("album");
    q.AddField("artist");
    q.AddField("genre");

    std::string where = generateWhereClause("genre", genre);
    q.AddWhereClause(where);

    where = generateWhereClause("artist", artist);
    q.AddWhereClause(where);

    where = generateWhereClause("album", album);
    q.AddWhereClause(where);

    where = generateWhereClause("title", title);
    q.AddWhereClause(where);

    q.AddWhereClause("available=1");

    q.SetOptionClause("ORDER BY title ASC");

    cResults result = reader->Query("metadata", q);

    // copy the titles into one vector from 'result'
    std::vector<cFileInfo> g;
    g.reserve(result.results.size());
    std::vector<cRow>::const_iterator it = result.results.begin();
    for ( ; it != result.results.end(); ++it)
    {
        if (it->size() != 7 /*number of 'AddField()' */)
            continue; // error

        cFileInfo file;
        /* in the same order as AddField() */
        file.path       = it->at(0);
        file.filename   = it->at(1);
        file.duration   = Atol(it->at(2));

        file.title      = it->at(3);
        file.album      = it->at(4);
        file.artist     = it->at(5);
        file.genre      = it->at(6);

        g.push_back(file);
    }

    return g;
}


std::vector<std::string> cSearchDatabase::Title(const std::string& genre, const std::string& artist, const std::string& album, const std::string& title)
{

    if (!reader)
        return std::vector<std::string>();

    cQuery q;
    q.AddField("title");

    std::string where = generateWhereClause("genre", genre);
    q.AddWhereClause(where);

    where = generateWhereClause("artist", artist);
    q.AddWhereClause(where);

    where = generateWhereClause("album", album);
    q.AddWhereClause(where);

    where = generateWhereClause("title", title);
    q.AddWhereClause(where);

    q.AddWhereClause("available=1");

    q.SetOptionClause("ORDER BY title ASC");

    cResults result = reader->Query("metadata", q);

    // copy the titles into one vector from 'result'
    std::vector<std::string> g;
    g.reserve(result.results.size());
    std::vector<cRow>::const_iterator it = result.results.begin();
    for ( ; it != result.results.end(); ++it)
    {

        if (it->size())
            g.push_back(it->at(0));
    }

    return g;
}

std::vector<std::string> cSearchDatabase::Tracks(const std::string& genre, const std::string& artist, const std::string& album, const std::string& filename)
{
    
    if (!reader)
        return std::vector<std::string>();
    
    cQuery q;
    q.AddField("distinct path"); // hack
    
    std::string where = generateWhereClause("genre", genre);
    q.AddWhereClause(where);
    
    where = generateWhereClause("artist", artist);
    q.AddWhereClause(where);
    
    where = generateWhereClause("album", album);
    q.AddWhereClause(where);
    
    where = generateWhereClause("path", filename);
    q.AddWhereClause(where);
    
    q.AddWhereClause("available=1");

    q.SetOptionClause("ORDER BY path ASC");

    cResults result = reader->Query("metadata", q);
    
    // copy the paths into one vector from 'result'
    std::vector<std::string> g;
    g.reserve(result.results.size());
    std::vector<cRow>::const_iterator it = result.results.begin();
    for ( ; it != result.results.end(); ++it)
    {
        
        if (it->size())
            g.push_back(it->at(0));
    }
    
    return g;
}

std::vector<std::string> cSearchDatabase::Albums(const std::string& genre, const std::string& artist, const std::string& album)
{
    if (!reader)
        return std::vector<std::string>();
    
    cQuery q;
    q.AddField("distinct album"); // hack
    
    std::string where = generateWhereClause("genre", genre);
    q.AddWhereClause(where);
    
    where = generateWhereClause("artist", artist);
    q.AddWhereClause(where);
    
    where = generateWhereClause("album", album);
    q.AddWhereClause(where);

    q.AddWhereClause("available=1");

    q.SetOptionClause("ORDER BY album ASC");

    cResults result = reader->Query("metadata", q);
    
    // copy the genres into one vector from 'result'
    std::vector<std::string> g;
    g.reserve(result.results.size());
    std::vector<cRow>::const_iterator it = result.results.begin();
    for ( ; it != result.results.end(); ++it)
    {
        
        if (it->size())
            g.push_back(it->at(0));
    }
    
    return g;
    
}

std::vector<std::string> cSearchDatabase::Artists(const std::string& genre, const std::string& artist)
{
    if (!reader)
        return std::vector<std::string>();
    
    cQuery q;
    q.AddField("distinct artist"); // hack
    
    std::string where = generateWhereClause("genre", genre);
    q.AddWhereClause(where);
    
    where = generateWhereClause("artist", artist);
    q.AddWhereClause(where);

    q.AddWhereClause("available=1");

    q.SetOptionClause("ORDER BY artist ASC");

    cResults result = reader->Query("metadata", q);
    
    // copy the genres into one vector from 'result'
    std::vector<std::string> g;
    g.reserve(result.results.size());
    std::vector<cRow>::const_iterator it = result.results.begin();
    for ( ; it != result.results.end(); ++it)
    {
        
        if (it->size())
            g.push_back(it->at(0));
    }
    
    return g;
}


std::vector<std::string> cSearchDatabase::Genres(const std::string &genre)
{
    if (!reader)
        return std::vector<std::string>();
    
    std::string where = generateWhereClause("genre", genre);

    cQuery q;
    q.AddField("distinct genre"); // hack
    q.AddWhereClause(where);

    q.AddWhereClause("available=1");

    q.SetOptionClause("ORDER BY genre ASC");

    cResults result = reader->Query("metadata", q);
    
    // copy the genres into one vector from 'result'
    std::vector<std::string> g;
    g.reserve(result.results.size());
    std::vector<cRow>::const_iterator it = result.results.begin();
    for ( ; it != result.results.end(); ++it)
    {
        
        if (it->size())
            g.push_back(it->at(0));
    }
    
    return g;
}

