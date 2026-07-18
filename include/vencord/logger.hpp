#pragma once

#include <memory>
#include <format>

namespace vencord
{
    struct logger
    {
        enum class level : std::uint8_t
        {
            trace,
            debug,
            info,
            warn,
            error,
        };

      private:
        struct impl;

      private:
        std::unique_ptr<impl> m_impl;

      private:
        logger();

      private:
        void log(level, std::string_view) const;

      public:
        template <typename... Ts>
        void operator()(std::format_string<Ts...>, Ts &&...) const;

        template <typename... Ts>
        void operator()(level, std::format_string<Ts...>, Ts &&...) const;

      public:
        [[nodiscard]] static logger &get();
    };
} // namespace vencord

#include "logger.inl"
