#ifndef I_TOOL_HPP
#define I_TOOL_HPP

#include "Utils/AObject.hpp"
#include <memory>

class ITool
{
public:
    virtual ~ITool() = default;

    // NECESSARY METHODS (USER NEEDS TO DEFINE THEM ALWAYS)
    virtual const char*              name()         = 0;
    virtual std::unique_ptr<AObject> settingPanel() = 0;

    // HELPFUL METHODS THAT ARE NOT NECESSARY TO OVERRIDE
    // LEAVE THEM EMPTY BY DEFAULT, SO USER CAN OVERRIDE THEM
    // IF HE WANTS TO
    virtual void activate()
    {
    }
    virtual void deactivate()
    {
    }

public:
};

#endif
