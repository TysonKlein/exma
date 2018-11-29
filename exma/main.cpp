#include <iostream>
int main(int ac, char* av[])
{
	std::cout << "command line arguments:\n";
	for (int i(1); i != ac; ++i)
		std::cout << i << "=====" << av[i] << "\n";
}