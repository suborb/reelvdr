#ifndef SEARCHAPI_H
#define SEARCHAPI_H

#include <string>
#include <deque>


class cFileInfo
{
public:
    std::string title;
    std::string artist;
    std::string genre;
    std::string album;

    std::string path;  // absolute path
    std::string filename; // name of the file

    unsigned long duration;
    // file size

    // last played
    // added date/time
};


// forward declarations
class cDatabase;
class cDatabaseReader;

class cSearchDatabase
{
    cDatabase *db;
    cDatabaseReader *reader;
    
protected:
    /* escape single quotes and replace '*' with '%' */
    std::string toSqlWildCardString(const std::string& str);
    std::string generateWhereClause(std::string column, std::string str);
    
public:
    cSearchDatabase(std::string& dbPath);
    ~cSearchDatabase();

    cFileInfo FileInfo(const std::string& path);
    
    std::deque<cFileInfo> Files(
            const std::string& genre,
            const std::string& artist,
            const std::string& album,
            const std::string& title
            );
    /**
     * @brief Title
     *      get a list of titles of files that match the given genre, artist, album, 'title'
     *      params match ignoring case and accpet wildcard '*'
     * @param genre
     * @param artist
     * @param album
     * @param title
     * @return
     */
    std::deque<std::string> Title(
            const std::string& genre,
            const std::string& artist,
            const std::string& album,
            const std::string& title
            );

    /**
     * @brief Tracks 
     *      get a list of paths that match the given genre, artist, album, filename
     *      params match ignoring case and accpet wildcard '*'
     * @param genre         
     * @param artist
     * @param album
     * @param filename
     * @return 
     */
    std::deque<std::string> Tracks(
            const std::string& genre, 
            const std::string& artist, 
            const std::string& album, 
            const std::string& filename);

    /**
     * @brief Albums 
     *          returns a list of albums that match given genre, artist and album strings
     *          params match ignoring case and accpet wildcard '*'. empty string matches all.
     * @param genre
     * @param artist
     * @param album
     * @return 
     */
    std::deque<std::string> Albums(const std::string& genre, 
                                    const std::string& artist, 
                                    const std::string& album);
    
    /**
     * @brief Artists
     *              returns a list of artist that match given genre and artist strings
     *              params match ignoring case and accpet wildcard '*'. empty string matches all.
     * @param genre
     * @param artist
     * @return 
     */
    std::deque<std::string> Artists(const std::string& genre, const std::string& artist);
    
    /**
     * @brief Genres
     *              returns a list of genre that match given genre string
     *              params match ignoring case and accpet wildcard '*'. empty string matches all.
     * @param genre
     * @return 
     */
    std::deque<std::string> Genres(const std::string& genre);
};

#endif // SEARCHAPI_H
