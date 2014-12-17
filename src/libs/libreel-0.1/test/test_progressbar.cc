#include <reel/tools.h>
#include <iostream>

int main()
{
	std::cout << "21% " <<  Reel::Tools::ProgressBarString(21) << std::endl;
	std::cout << "55% " <<  Reel::Tools::ProgressBarString(55) << std::endl;
	std::cout << "1% "  <<  Reel::Tools::ProgressBarString(1) << std::endl;
	std::cout << "99% " <<  Reel::Tools::ProgressBarString(99) << std::endl;
	std::cout << "199% " <<  Reel::Tools::ProgressBarString(199) << std::endl;
	std::cout << "-19% " <<  Reel::Tools::ProgressBarString(-19) << std::endl;

	return 0;
}
