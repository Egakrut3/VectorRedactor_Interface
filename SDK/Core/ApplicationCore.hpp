#ifndef APPLICATION_CORE_HPP
#define APPLICATION_CORE_HPP

#include "Interfaces/IID.hpp"
#include "Interfaces/IInputPump.hpp"
#include "Interfaces/IRenderer.hpp"
#include "Interfaces/Package.hpp"
#include "Utils/AObject.hpp"

#include <cassert>
#include <dlfcn.h>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

class ApplicationCore
{
private:
    ApplicationCore();
    ~ApplicationCore();

    std::unordered_map<IID, void*> registry_;
    std::unique_ptr<AObject>       root_;
    std::vector<void*>             package_handles_;
    bool                           is_running_ = false;

public:
    ApplicationCore(const ApplicationCore&)            = delete;
    ApplicationCore(ApplicationCore&&)                 = delete;
    ApplicationCore& operator=(const ApplicationCore&) = delete;
    ApplicationCore& operator=(ApplicationCore&&)      = delete;

    enum class Error;

    static ApplicationCore& instance();
    AObject*                root() const;
    Error                   queryInterface(IID iid, void** interface) const;

    Error run();
    void  stop();

    Error registerInterface(IID iid, void* interface);
    Error loadPackage(const std::string& path);
};

enum class ApplicationCore::Error
{
    SUCCESS,
    INVALID_ARGS,

    INTERFACE_NOT_FOUND,
    APPLICATION_ALREADY_RUNNING,
    NOT_LOADED_RENDERER,
    NOT_LOADED_INPUT_PUMP,
    INTERFACE_ALREADY_EXISTS,
    PACKAGE_NOT_FOUND,
    PACKAGE_INVALID_BINARY,
};

/* ======================== Implementation ======================== */

ApplicationCore::ApplicationCore()
{
    root_ = std::make_unique<AObject>();
}

ApplicationCore::~ApplicationCore()
{
    registry_.clear();

    for (auto handle = package_handles_.rbegin(); handle != package_handles_.rend(); ++handle)
    {
        if (*handle != nullptr)
        {
            dlclose(*handle);
        }
    }

    package_handles_.clear();
}

ApplicationCore& ApplicationCore::instance()
{
    static ApplicationCore instance;
    return instance;
}

AObject* ApplicationCore::root() const
{
    return root_.get();
}

ApplicationCore::Error ApplicationCore::queryInterface(IID iid, void** interface) const
{
    if (interface == nullptr)
    {
        return Error::INVALID_ARGS;
    }

    auto found = registry_.find(iid);
    if (registry_.contains(iid))
    {
        *interface = found->second;
    }

    return Error::INTERFACE_NOT_FOUND;
}

ApplicationCore::Error ApplicationCore::run()
{
    /* already running => return */
    if (is_running_)
    {
        return Error::APPLICATION_ALREADY_RUNNING;
    }

    IRenderer* renderer = nullptr;
    if (queryInterface(IID_RENDERER, reinterpret_cast<void**>(&renderer)) != Error::SUCCESS)
    {
        return Error::NOT_LOADED_RENDERER;
    }
    assert(renderer);

    IInputPump* input_pump = nullptr;
    if (queryInterface(IID_INPUT_PUMP, reinterpret_cast<void**>(&input_pump)) != Error::SUCCESS)
    {
        return Error::NOT_LOADED_INPUT_PUMP;
    }
    assert(input_pump);

    is_running_ = true;
    while (is_running_)
    {
        auto polled = input_pump->pollEvent();
        if (polled.has_value())
        {
            root_->processEvent(polled.value());
        }

        root_->render();
        renderer->update();
    }

    return Error::SUCCESS;
}

void ApplicationCore::stop()
{
    is_running_ = false;
}

ApplicationCore::Error ApplicationCore::registerInterface(IID iid, void* interface)
{
    if (interface == nullptr)
    {
        return Error::INVALID_ARGS;
    }

    if (registry_.contains(iid))
    {
        return Error::INTERFACE_ALREADY_EXISTS;
    }

    registry_[iid] = interface;
    return Error::SUCCESS;
}

ApplicationCore::Error ApplicationCore::loadPackage(const std::string& path)
{
    void* handle = dlopen(path.c_str(), RTLD_NOW);
    if (handle == nullptr)
    {
        return Error::PACKAGE_NOT_FOUND;
    }

    InitPackageFunc* init_package = reinterpret_cast<InitPackageFunc*>(dlsym(handle, "initPackage"));
    if (init_package == nullptr)
    {
        dlclose(handle);
        return Error::PACKAGE_INVALID_BINARY;
    }

    // TODO: maybe add return code for initPackage
    init_package(this);
    package_handles_.push_back(handle);

    return Error::SUCCESS;
}

#endif
