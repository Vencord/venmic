#include "logger.hpp"

#include <spdlog/sinks/ansicolor_sink.h>
#include <spdlog/sinks/basic_file_sink.h>

namespace vencord
{
    struct logger::impl
    {
        std::unique_ptr<spdlog::logger> logger;
    };

    logger::logger() : m_impl(std::make_unique<impl>())
    {
        namespace sinks = spdlog::sinks;

        auto file_sink = std::make_shared<sinks::basic_file_sink_mt>("venmic.log");
        file_sink->should_log(spdlog::level::trace);
        file_sink->set_level(spdlog::level::trace);

        auto stdout_sink = std::make_shared<sinks::ansicolor_stdout_sink_mt>();
        stdout_sink->set_level(spdlog::level::info);

        m_impl->logger = std::make_unique<spdlog::logger>("venmic", spdlog::sinks_init_list{stdout_sink, file_sink});
    }

    spdlog::logger *logger::operator->() const
    {
        return m_impl->logger.get();
    }

    logger &logger::get()
    {
        static std::unique_ptr<logger> instance;

        if (!instance)
        {
            instance = std::unique_ptr<logger>(new logger);
        }

        return *instance;
    }
} // namespace vencord
