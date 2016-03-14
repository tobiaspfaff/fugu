#include "util.hpp"
#include <iostream>
using namespace std;

void fatalError(const string& msg)
{
	cout << "FATAL ERROR (VULKAN): " << msg << endl;
	exit(1);
}

