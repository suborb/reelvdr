#include "disk_tools.h"
#include <iostream>
#include <error.h>

int main (int argc, char* argv[])
{
    if (argc != 2)
        error (EXIT_FAILURE, 0, "wrong number of arguments");

    cPartitions partitions;
    bool result = GetPartitions(argv[1], partitions);    
    if (result) 
    {
        if(partitions.size()) std::cout << std::endl;
        cPartitions::iterator it = partitions.begin();
        for ( ; it != partitions.end(); ++it)
        {
            std::cout<< it->ToString() << std::endl;
        } // for
    } else
    {
        std::cout << "Error: could not read parition info of "<< argv[1] << std::endl;
    }

    return 0;
}
