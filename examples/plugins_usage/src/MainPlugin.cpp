#include "MainPlugin.hpp"
#include "Interfaces/IRenderer.hpp"
#include "Interfaces/Package.hpp"
#include "Core/ApplicationCore.hpp"
#include <cassert>
#include <memory>
#include <variant>

extern "C" bool INIT_PACKAGE_FUNC_NAME(ApplicationCore *core)
{
	assert(core);

	core->registerInterface(IID_MAIN_PLUGIN,
			reinterpret_cast<void *>(&IMainPlugin::instance()));

	g_application_core = core;

	auto main_window = std::make_unique<MainWindow>();
	core->root()->addChild(std::move(main_window));

	return true;
};

void MainWindow::render()
{
	IRenderer *renderer = nullptr;
	if(g_application_core->queryInterface(IID_RENDERER,
			reinterpret_cast<void **>(&renderer)) != ApplicationCore::Error::SUCCESS)
	{
		return;
	}

	renderer->renderRect(Color { .r = 0x00, .g = 0xFF, .b = 0x00, .a = 0xFF }, BoxF { 100.0F, 100.0F, 200.0F, 200.0F }, 0.0F);
}

bool MainWindow::processEvent(const Event &event)
{
	if(std::holds_alternative<EventQuit>(event))
	{
		g_application_core->stop();
		return true;
	}

	return true;
}

IMainPlugin& IMainPlugin::instance()
{
	static IMainPlugin instance;
	return instance;
}

