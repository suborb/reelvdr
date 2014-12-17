#include "analyzer.h"
#include "tools.h"
#include <iostream>
#include <taglib/tag.h>
#include <taglib/fileref.h>
#include <sstream>
#include "sqlutils.h"
#include <unistd.h>

cMutex taglibMutex;

std::string toString(const TagLib::String& s)
{
    if (s.isNull() || s.isEmpty())
        return "";

    std::stringstream ss;
    ss << s.toCString(true);
    return ss.str();
}

typedef std::vector<std::string> cFields;
typedef std::vector<std::string> cRow;


bool MetaData(const char* filepath, cFields& fields, cRow& props)
{
#define ADD(x, t) do { if (t) { fields.push_back(x); props.push_back(t); } } while(0)
#define ADDSTRING(x, t) do { if (!t.empty()) { fields.push_back(x); props.push_back(t); } } while(0)
#define ADDINT(x, t) do { std::string c = *itoa(t); if (!c.empty()){ fields.push_back(x); props.push_back(c);} } while(0)
    
    cMutexLock mutexLock(&taglibMutex); // taglib is not multithread safe, protect by mutex
    TagLib::FileRef f(filepath);

    fields.clear();
    props.clear();

    //cTag t;
    //const char* tmp = NULL;
    std::string tmp;
    
    if (f.isNull()) 
    {
        std::cerr<< "Taglib failed on " << filepath << std::endl;
        ADD("metadata_needs_update", "2"); // failure
        return false;
    }
   
    TagLib::Tag *tag = f.tag();
    
    if (tag) 
    {
#if 0
      tmp   = toString(tag->title()).c_str();   ADD("title",   tmp);
      tmp   = toString(tag->artist()).c_str();  ADD("artist",  tmp);
      tmp   = toString(tag->album()).c_str();   ADD("album",   tmp);
      tmp   = toString(tag->comment()).c_str(); ADD("comment", tmp);
      tmp   = toString(tag->genre()).c_str();   ADD("genre",   tmp);
#else
        tmp   = toString(tag->title());   ADDSTRING("title",   tmp);
        tmp   = toString(tag->artist());  ADDSTRING("artist",  tmp);
        tmp   = toString(tag->album());   ADDSTRING("album",   tmp);
        tmp   = toString(tag->comment()); ADDSTRING("comment", tmp);
        tmp   = toString(tag->genre());   ADDSTRING("genre",   tmp);
#endif
      
      //ADDINT("track", tag->track());
      ADDINT("year",  tag->year());
    }
    
    TagLib::AudioProperties *prop = f.audioProperties();
    if (prop) 
    {
        ADDINT("bitrate",    prop->bitrate());
        ADDINT("samplerate", prop->sampleRate());
      //  ADDINT("channels",   prop->channels());
        ADDINT("length" ,    prop->length());
    }
 
    ADD("metadata_needs_update", "0");
    return true;
}

/*
** cAnalyzer 
** 
** extract metadata from a given path in a seperate thread
*/

cAnalyzer::cAnalyzer(cDatabaseWriter *w, std::string name) 
    :cThread(name.c_str()),
      writer(w)
{
}

void cAnalyzer::SetPath(const cPath& p)
{
    cThreadLock theadLock(this);
    path = p;
}

void cAnalyzer::Action()
{
    cThreadLock threadLock(this);
    
    
    if (access(path.path.c_str(), R_OK) != 0)
        return;
    cFields field;
    cRow props;
    
    bool ok = MetaData(path.path.c_str(), field, props);
    // XXX metadata extraction failed... what to do ?
    if (true || ok)
    {
        std::string where = "path=" + QuoteString(path.path);
        writer->Update(METADATA_TABLE, field, props, where);
    }
}


/*
** cAnalyzerManager 
**
** Reads database to get yet unanalysed files and 
** starts cAnalyzer threads for those.
**
*/
cAnalyzerManager::cAnalyzerManager(cDatabaseReader *r, cDatabaseWriter *w, unsigned int num)
    : cThread("Analyzer Manager"),
      num_threads(MAX_CRAWLERS < num? MAX_CRAWLERS: num),
      reader(r),
      writer(w)
{
    analyzedFilesCount = 0;
    reanalyze          = false;
    
    for (unsigned int i = 0; i < num_threads; ++i)
        analyzers[i] = new cAnalyzer(writer, *cString::sprintf("Analyzer %d", i));
}

unsigned long long cAnalyzerManager::AnalyzedFileCount() const 
{ 
    return analyzedFilesCount; 
}

void cAnalyzerManager::SetReanalyze(bool on)
{
    reanalyze = on;
}

bool cAnalyzerManager::Reanalyze() const
{
    return reanalyze;
}


std::queue<std::string>::size_type cAnalyzerManager::QueueSize() const 
{ 
    return Queue.size(); 
}

bool cAnalyzerManager::AllActive() const
{
    for (unsigned int i = 0; i < num_threads; ++i)
        if (analyzers[i] && !analyzers[i]->Active())
            return false;
    
    return true;
}

int cAnalyzerManager::CountActive() const
{
    int count = 0;
    for (unsigned int i = 0; i < num_threads; ++i)
        if (analyzers[i]  && analyzers[i]->Active()) 
            ++count;
    
    return count;
}

void cAnalyzerManager::Stop(int waitSec)
{
    Cancel(waitSec);
}

unsigned long long cAnalyzerManager::StillToUpdateCount() const
{
    unsigned long long count = 0;
    cQuery q;
    q.AddField("count(path)"); // hack
    q.AddWhereClause("available=\"1\" AND metadata_needs_update=\"1\"");
    
    cResults result = reader->Query(METADATA_TABLE, q);
    
    if (result.results.size() && result.results[0].size())
        count = atoll(result.results[0].at(0).c_str());
    
    return count;
}

void cAnalyzerManager::Action()
{
    // analyze all files even those that were previous analyzed ?
    if (Reanalyze())
    {
        std::cerr << "REANALYZE" << std::endl;
        // set metadata_needs_update = 1 for all files
        cFields field;
        cRow    row;
        std::string where = "path like '%'"; // if where.empty(), Update() is not executed!

        field.push_back("metadata_needs_update");
        row.push_back("1");
        writer->Update(METADATA_TABLE, field, row, where);
    }

    while (Running()) 
    {
        cCondWait::SleepMs(10);
        
        // All threads are currently active
        // sleep for a while and check again
        if (AllActive())
        {
            continue;
        }
        
        if (Queue.empty())
        {
            // read from database MAX_CRAWLERS number of paths that need
            // metadata update
            cQuery q;
            q.AddField("path"); 
            static unsigned int offset = 0;
            std::string OFFSET = std::string(" OFFSET ") + *itoa(offset);
            q.AddWhereClause(std::string("available=\"1\" AND metadata_needs_update=\"1\" LIMIT 1000")
                             + OFFSET );
            //offset += 100;
            //std::cerr << "Offset "  << offset << std::endl;
            cResults result = reader->Query(METADATA_TABLE, q);
            
            if (result.results.size())
            {
#if 0
                // SetIOPriority() gives "Invalid argument" error
                SetIOPriority(0);
                SetPriority(0);
#endif
                std::vector<cRow>::const_iterator it;
                for (it = result.results.begin();
                     it != result.results.end();
                     ++it)
                {
                    cRow row = *it;
                    if (row.size()) 
                    {
                        //std::cerr << *(row.begin()) << std::endl;
                        Queue.push(*(row.begin()));
                    }
                }
            } else 
            {
#if 0
                std::cerr << "No more data..." << std::endl;
                // no file needing metadata update found 
                // lower priority of thread
                SetIOPriority(19);
                SetPriority(19);
#endif
                cCondWait::SleepMs(1000);
            }
        }
        
        for (unsigned int i = 0; !Queue.empty() && i < num_threads; ++i)
        {
            if (analyzers[i] && !analyzers[i]->Active()) 
            {
                cPath p; p.linkLevel = 0; p.path = Queue.front();
                analyzers[i]->SetPath(p);
                analyzers[i]->Start();
                
                // remove path from queue
                Queue.pop();
                
                analyzedFilesCount ++;
            }
        } // for
        
    } // while
} // Action()
