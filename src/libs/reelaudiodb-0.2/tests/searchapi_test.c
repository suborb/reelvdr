#include "searchapi.h"
#include <iostream>

int main(int argc, char* argv[])
{
    if (argc < 2)
    {
         std::cout << "Usage:\n\t" << " path_to_database  [genre] [artist] [album] [title]"  << std::endl;
         return 0;
    }

    std::string dbPath(argv[1]);
    cSearchDatabase search(dbPath);
    
    std::vector<std::string> result ;
    switch (argc) 
    {
        case 1:
        case 2: 
        case 3:
            std::cout << "Genres:" << std::endl;
            result = search.Genres(argc > 2 ? argv[2]:"");
            break;
            
        case 4:
            std::cout << "Artist:" << std::endl;
            result = search.Artists(argv[2], argv[3]);
            break;
            
        
        case 5:
            std::cout << "Album:" << std::endl;
            result = search.Albums(argv[2], argv[3], argv[4]);
            break;
            
        default:
        case 6:
            std::cout << "Title:" << std::endl;
            result = search.Title(argv[2], argv[3], argv[4], argv[5]);
            break;
    }
    
    for (unsigned int i=0; i<result.size(); ++i)
        std::cout << "'" << result.at(i) << "'" << std::endl;
    return 0;
}


