#include <fstream>
#include <iostream>

#include "Playlist.h"
#include <stdio.h>

vector<string> ReadPlaylist(string filename)
{
    ifstream f(filename.c_str());
    vector<string> playlist;
    string line;

    if (f.is_open()) 
    {
        while (f.good()) 
        {
            getline(f, line);

            if (line.size() > 0)
                playlist.push_back(line);
        } // while

        f.close();
    } // if f.is_open()

    return playlist;
}


bool SavePlaylist(string filename, vector<string> playlist)
{
    // cannot save to a file with empty filename
    if (!filename.size()) return false;
    
    printf("SavePlaylist('%s', %d size)\n", filename.c_str(), playlist.size());
    ofstream f(filename.c_str());

    if (!f.is_open())
    {
        std::cout << "\033[0;91m Could not open " << filename << "\033[0m" << std::endl;
        return false;
    }
    
    vector<string>::iterator it = playlist.begin();
    
    while (it != playlist.end()) 
    {
        if (!f.good()) return false;

        f << *it << endl;
        ++it;
    } // while

    f.close();

    return true;
}


