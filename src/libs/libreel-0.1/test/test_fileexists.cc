#include <reel/tools.h>
#include <iostream>

int main(int argc, char* argv[])
{
	for (unsigned int i = 1; i < argc ; ++i)
		std::cout << argv[i] << " " 
			<< (Reel::Tools::FileExists(argv[i])?"Exists": "not found") 
			<< std::endl;

	return 0;
}
