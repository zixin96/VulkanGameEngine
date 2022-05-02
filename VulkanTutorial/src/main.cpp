#include "HelloTriangleApplication.h"

// these two headers are for reporting and propagating errors
#include <iostream>
#include <stdexcept>

int main()
{
	HelloTriangleApplication app;

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
