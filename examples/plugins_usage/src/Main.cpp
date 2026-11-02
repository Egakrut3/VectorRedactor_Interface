#include "Core/ApplicationCore.hpp"
#include "MainPlugin.hpp"

int main()
{
	ApplicationCore& app = ApplicationCore::instance();

	if(app.loadPackage("./MainPlugin.so") != ApplicationCore::Error::SUCCESS)
	{
		ApplicationCore::log(ApplicationCore::LogLevel::CRITICAL,
				"MainPlugin loading failed!");
		return 1;
	}
	ApplicationCore::log(ApplicationCore::LogLevel::OK, "MainPlugin successfully loaded");

	if(app.loadPackage("./GUIEngine.so") != ApplicationCore::Error::SUCCESS)
	{
		ApplicationCore::log(ApplicationCore::LogLevel::CRITICAL,
				"GUIEngine loading failed!");
		return 1;
	}
	ApplicationCore::log(ApplicationCore::LogLevel::OK, "GUIEngine successfully loaded");

	app.run();
	return 0;
}

