#include <reel/tools.h>
#include <iostream>

int main(int argc, char* argv[])
{
	std::cout << "wlan0" << (Reel::Tools::IsNetworkDevicePresent("wlan0")?" Yes": " No") << std::endl;
	std::cout << "eth2" << (Reel::Tools::IsNetworkDevicePresent("eth2")?" Yes": " No") << std::endl;

	for (unsigned int i = 1; i < argc ; ++i)
		std::cout << argv[i] 
			<< (Reel::Tools::IsNetworkDevicePresent(argv[i])?" Yes": " No") 
			<< std::endl;
	return 0;
}
