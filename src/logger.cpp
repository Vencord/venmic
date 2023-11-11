#include "logger.hpp"

#include <filesystem>
#include <spdlog/sinks/ansicolor_sink.h>
#include <spdlog/sinks/basic_file_sink.h>

namespace vencord
{
    namespace fs = std::filesystem;

    struct logger::impl
    {
        std::unique_ptr<spdlog::logger> logger;
    };

    fs::path log_directory()
    {
        auto rtn = fs::temp_directory_path();

        if (auto home = std::getenv("HOME"))
        {
            rtn = home / ".local" / "state";
        }

        if (auto state_home = std::getenv("XDG_STATE_HOME"))
        {
            rtn = state_home;
        }

        return rtn / "venmic";
    }

    logger::logger() : m_impl(std::make_unique<impl>())
    {
        namespace sinks = spdlog::sinks;

        m_impl->logger = std::make_unique<spdlog::logger>("venmic");
        m_impl->logger->set_level(spdlog::level::trace);
        m_impl->logger->flush_on(spdlog::level::trace);

        auto stdout_sink = std::make_shared<sinks::ansicolor_stdout_sink_mt>();

        stdout_sink->set_level(spdlog::level::info);
        m_impl->logger->sinks().emplace_back(stdout_sink);

        if (!std::getenv("VENMIC_ENABLE_LOG"))
        {
            return;
        }

        auto log_file = log_directory() / "venmic.log";

        if (!fs::exists(log_file))
        {
            [[maybe_unused]] std::error_code ec;
            fs::create_directories(log_file.parent_path(), ec);
        }

        auto file_sink = std::make_shared<sinks::basic_file_sink_mt>(log_file.string());

        file_sink->set_level(spdlog::level::trace);
        m_impl->logger->sinks().emplace_back(file_sink);
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
