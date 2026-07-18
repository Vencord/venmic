#pragma once

#include "logger.hpp"

namespace vencord
{
    template <typename... Ts>
    void logger::operator()(logger::level level, std::format_string<Ts...> format, Ts &&...args) const
    {
        return log(level, std::format(format, std::forward<Ts>(args)...));
    }

    template <typename... Ts>
    void logger::operator()(std::format_string<Ts...> format, Ts &&...args) const
    {
        return log(level::info, std::format(format, std::forward<Ts>(args)...));
    }
} // namespace vencord
