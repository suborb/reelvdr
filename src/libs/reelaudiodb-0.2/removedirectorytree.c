#include "createdatabaseinstance.h"
#include "databasewriter.h"
#include <iostream>
#include <getopt.h>

/*
 * make sure the first and last char of argv[1] is '/'
 * - absolute path
 * - directory
 */
bool DirPathOkay(const char* path)
{
    if (!path) return false;
    std::cerr << "'" << path << "'" << std::endl;
    if (*path != '/') return false; // not abs path

    // find the end of the string
    while (*++path);

    return *--path == '/';
}


std::string EscapeSingleQuotes(const char* str)
{
    if (!str)
        return std::string();

    std::string s = str;
    std::string result;
    for (unsigned int i = 0; i < s.size(); ++i)
    {
        // escape single quotes with a '\'
        if (s.at(i) == '\'')
            result.push_back('\\');

        result.push_back(s.at(i));
    }

    return result;
}

void PrintUsage(int argc, char*argv[])
{
    std::cerr << "Useage:\n\t" << argv[0] << " --database <databasename> " << " --directory-tree <abs. path containing '/' at the end>"
              << std::endl;
}

/**
 * @brief mark as unavailable all files under the given tree
 * @param --database <dbname>
 * @param --directory-tree <path>
 * @return
 */

int main(int argc, char*argv[])
{
    static struct option long_options[] =
    {
        {"database",  required_argument,       0,                 'd'         },
        {"directory-tree",  required_argument, 0,                 't'         },
        { NULL,       0,                     NULL,                 0          }
    };


    int c = -1;
    int option_index = 0;

    std::string dirTree;
    std::string dbPath;
    
    while ( 1 )
    {
        c = getopt_long(argc, argv, "d", long_options, &option_index);

        /* no more options */
        if (c == -1)
            break;

        switch ( c )
        {
        case 0:
            break;

        case 'd':
            dbPath = optarg;
            break;
            
        case 't':
                dirTree = optarg;
                break;

        default:
            break;
        } // switch
    } // while
    
    if (dirTree.empty() || dbPath.empty())
    {
        PrintUsage(argc, argv);
        return 1;
    }
    
    /*
     * make sure the first and last char of argv[1] is '/'
     * - absolute path
     * - directory
     */
    if (!DirPathOkay(dirTree.c_str()))
    {
        std::cerr << "Directory path must be absolute and end in '/'" << std::endl;
        return 1;
    }

    cDatabase *db = CreateDatabaseInstance(dbPath);
    cDatabaseWriter writer(db);

    cFields fields;
    cRow row;

    fields.push_back("available");
    row.push_back("0");

    std::string where = std::string("path like '") + EscapeSingleQuotes(dirTree.c_str()) + std::string("%'");
    bool result = writer.Update("metadata", fields, row, where);

    std::cerr << (result?"Success":"Failure") << std::endl;

    return 0;
}
