
#include <iostream>
#include "FirstApp.h"

int main()
{
	ZZX::FirstApp app{};

	try
	{
		app.run();
	}
	// catch standard exception types
	catch (const std::exception& e)
	{
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
