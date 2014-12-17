
#ifndef __MMFILES_H
#define __MMFILES_H

#include <vector>
#include <string>
#include "thread.h"

// recursively scan subdirs and files in these paths
//
// Singleton class: only one instance of this class should be available
class cDirList
{

    private:

        // List of dirs to scan recursively from
        // cScanDir thread should read from this list
        std::vector<std::string> PathsToScanFrom;
        bool changed_;

        // make all constructors private : sigleton class
        cDirList();
        cDirList(const cDirList&);
        cDirList& operator=(const cDirList&);

    public:

        static cDirList& Instance();

        bool AddPath    (const std::string& new_path);
        bool RemovePath (const std::string& old_path);
        
        unsigned int FindInList(const std::string& path);

        bool HasChanged() const;
        void ResetChanged();

        unsigned int ListSize()const;
        std::string  PathAt( unsigned int& i)const;     

        inline void ClearList() { PathsToScanFrom.clear();}

};

extern cScanDirThread *scandirthread;
#endif

