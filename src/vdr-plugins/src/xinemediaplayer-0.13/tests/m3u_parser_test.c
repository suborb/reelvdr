#define esyslog 

#include "m3u_parser.h"
#include <iostream>


int main(int argc, char* argv[])
{
	int i = 0;
	
	for ( i = 1; i < argc ; ++i)
	{
		std::vector <std::string> titles, list;
		std::cout << "parsing " << argv[i] << " ";
		m3u_parser(argv[i], list, titles);
		std::cout << "list.size:" << list.size()
			<< " titles.size:" << titles.size()
			<< std::endl;
	}


	return 0;

}
