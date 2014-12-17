#include "crawler.h"
#include "thread.h"
#include "tools.h"

/**
 * @brief The cDriCrawler class
 *  traverse ONE given directory for its contents
 *  add subdirectories to DirQ and files to DB
 */

cDirCrawler::cDirCrawler(cDatabaseWriter *w, cDirectoryQueue *queue, std::string name)
    : cThread(name.c_str()), 
      dirQ(queue),
      writer(w)
{
    
}

void cDirCrawler::SetDirectory(const cPath& path)
{
    cThreadLock threadLock(this);
    pwd = path;
}

bool IsIgnored(const char* fileName)
{
    static const char *allowedExtentions[] = { 
        ".mp3", ".ogg", 
        ".mkv", ".flac", 
        ".wav", ".mp4",
        ".a3c", NULL 
    };
    
    // ignore null filename
    if (!fileName) 
        return true;
    
    int fStrlen = strlen(fileName);
    int extStrlen = 0;
    for (int i=0; allowedExtentions[i]; ++i)
    {
        extStrlen = strlen(allowedExtentions[i]);
        if (extStrlen < fStrlen && strcasecmp(fileName+(fStrlen-extStrlen), allowedExtentions[i]) == 0)
            return false;
    }
    
    printf("ignore '%s'\n", fileName);
    return true;
}

void cDirCrawler::Action()
{
    cThreadLock threadLock(this);
    const char* DirName = pwd.path.c_str();
    int LinkLevel = pwd.linkLevel;
    isyslog("thread %d, Dir '%s'", ThreadId(), DirName);
    cReadDir d(DirName);
    struct dirent *e;
    
    while (Running() && (e = d.Next()) != NULL) 
    {
        // Ignore NULL or empty names and all hidden directories and files
        if (!e->d_name || !strlen(e->d_name) || e->d_name[0] == '.')
            continue;

        // construct full path
        cString buffer(AddDirectory(DirName, e->d_name));
        struct stat st;

        // check if path is a link
        if (lstat(buffer, &st) != 0)
            continue; // error

        if (S_ISLNK(st.st_mode))
        {
            ++LinkLevel;

            // too many symlink in this path, maybe cyclic
            if (LinkLevel > MAX_LINK_LEVEL)
            {
                isyslog("max link level exceeded - not scanning %s", *buffer);
                continue;
            }

            if (stat(buffer, &st) != 0)
                continue; // error
        }

        cPath nextPath;
        nextPath.path = *buffer;
        nextPath.linkLevel = LinkLevel;

        // if subdirectory, put it in the directory Queue for other threads
        if (S_ISDIR(st.st_mode))
        {
            isyslog("pwd: %s got '%s' , push '%s'", DirName, e->d_name, *buffer);
            dirQ->AddPath(nextPath);
        }
        else if (S_ISREG(st.st_mode))
        {
            nextPath.filename = e->d_name;
            // a file
            AddPathToDatabase(nextPath);
        }
    } // while
} // Action()


bool cDirCrawler::AddPathToDatabase(const cPath & path)
{
    // check if the following file is "allowed" 
    if (IsIgnored(path.path.c_str()))
        return false;
    
    std::deque<std::string> fields;
    fields.push_back("path");
    fields.push_back("available");
    
    std::deque<std::string> values;
    values.push_back(path.path); // abs. path
    values.push_back("1");       // available
    
    if (!path.filename.empty())
    {
        fields.push_back("filename");
        values.push_back(path.filename); // just the name of the file
    }

    std::string where = std::string("path=\"") + path.path + std::string("\"");
    
    // Insert or update fields
    //writer->Write(fields, values, where);
    
    //std::string sqlQuery = "INSERT INTO " + table + " (path, available) VALUES (" + path.path, "1 )";
    
    // INSERT 
    // if INSERT fails try  UPDATE
    // if both fail ==> error
    return writer->Write(METADATA_TABLE, fields, values, where);
}


//////////////////////////////////////////////////////////////////////////////////////
//// cCrawlerManager
//////////////////////////////////////////////////////////////////////////////////////

cCrawlManager::cCrawlManager(cDatabaseReader *r, cDatabaseWriter *w, unsigned int num) 
    : cThread(),
      reader(r), 
      writer(w), 
      num_threads(MAX_CRAWLERS < num? MAX_CRAWLERS: num)
{
    crawledDirsCount = 0;
    
    for (unsigned int i=0; i< num_threads; ++i)
        crawlers[i] = new cDirCrawler(writer, &dirQueue, *cString::sprintf("Crawler %d", i));
}

cCrawlManager::~cCrawlManager()
{
    Stop();
}

int cCrawlManager::CountActive() const
{
    int count = 0;
    for (unsigned int i = 0; i<num_threads; ++i)
        if (crawlers[i]->Active()) ++count;
    return count;
}

unsigned long long cCrawlManager::CrawledDirsCount() const
{
    return crawledDirsCount;
}

bool cCrawlManager::ScanDir(const std::string &dir)
{
    cPath newpath;
    newpath.path = dir;
    newpath.linkLevel = 0;
    
    return dirQueue.AddPath(newpath);
}

void cCrawlManager::Action()
{
    unsigned int i;
    while (Running())
    {
        if (dirQueue.Empty()) 
        {
            // if all crawlers are inactive when Q is empty
            // then nothing more to crawl.
            if (CountActive() == 0)
            {
                isyslog("crawl complete");
                return;
            }
            
            // Q is empty and some crawlers are active, wait for them
            // to fill up the Q and check again.
            cCondWait::SleepMs(10);
            continue;
        } // if dirQueue.Empty()
        
        
        // fill all threads
        for (i = 0; !dirQueue.Empty() && i<num_threads; ++i)
        {
            if (crawlers[i]->Active()) continue;

            crawlers[i]->SetDirectory(dirQueue.Pop());
            crawlers[i]->Start();
            
            crawledDirsCount++;
        } // for
        cCondWait::SleepMs(10);
        
    } // while
} // Action()

void cCrawlManager::Stop()
{
    // flag crawlerManager thread to stop
    Cancel(-1);

    // flag all crawl threads to stop
    for (unsigned int i=0 ; i < num_threads; ++i)
        crawlers[i]->Cancel(-1);

    // stop all threads
    Cancel(1);
    for (unsigned int i=0 ; i < num_threads; ++i)
        crawlers[i]->Cancel(1);

}
