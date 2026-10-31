#ifndef IINPUT_PUMP_HPP
#define IINPUT_PUMP_HPP

#include "Utils/Event.hpp"

#include <optional>

class IInputPump
{
public:
    IInputPump()          = default;
    virtual ~IInputPump() = default;

    virtual std::optional<Event> pollEvent() = 0;
};

#endif
