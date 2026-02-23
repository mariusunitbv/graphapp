#pragma once

#include <source_location>
#include <stdexcept>

#define GAPP_THROW(message)                                                                   \
    throw std::runtime_error(std::string(std::source_location::current().file_name()) + ":" + \
                             std::to_string(std::source_location::current().line()) + "\n" +  \
                             std::source_location::current().function_name() + "\n" + message)

#define FORWARD_DECLARE_CLASS(name) export class name
