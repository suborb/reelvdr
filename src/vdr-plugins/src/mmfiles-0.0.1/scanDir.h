#include <vector>
#include <string>

#include <sys/types.h>
#include <sys/stat.h>

#include <unistd.h>
#include <errno.h>
#include <dirent.h>

/**
 * Scans Directory structure for MultiMedia Files
 * 
 * options:
 * recursive, follow links
 */
class cScanDir
{
    private:
        bool followLink_;           // to ignore links or not
        bool recursive_;

    public:
        cScanDir();

        // Scan the path for files and subdirectories
        // is recursive depending upon Recursive()
        bool ScanDirTree(const std::string& path);

        // Get the contents of one directory : path
        // and fill files and subdirs vectors
        bool DirContents(const std::string& path, std::vector<std::string> &files, 
                         std::vector<std::string>& subdirs);
        void PrintDirContents(const std::string& path, const std::vector<std::string> &files, 
                         const std::vector<std::string>& subdirs)const;

        // set and get the variable recursive_
        void SetRecursive   (bool recur);
        bool Recursive      ()          const; 

        // set and get the variable followLink_
        void SetFollowLinki (bool followLink);
        bool FollowLink     ()          const;

        // checks if given path is a Directory
        bool IsDir          (const std::string& path) const;
        // checks if given path is a Link 
        bool IsLink         (const std::string& path) const;
        // checks if given path is a Regular File 
        bool IsFile         (const std::string& path) const;
};
