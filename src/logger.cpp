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

    static fs::path log_directory()
    {
        auto rtn = fs::temp_directory_path();

        // NOLINTNEXTLINE(*-mt-unsafe)
        if (auto *home = std::getenv("HOME"))
        {
            rtn = fs::path{home} / ".local" / "state";
        }

        // NOLINTNEXTLINE(*-mt-unsafe)
        if (auto *state_home = std::getenv("XDG_STATE_HOME"))
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

        // NOLINTNEXTLINE(*-mt-unsafe)
        if (!std::getenv("VENMIC_ENABLE_LOG"))
        {
            return;
        }

        auto directory = log_directory();

        if (!fs::exists(directory))
        {
            [[maybe_unused]] std::error_code ec;
            fs::create_directories(directory, ec);
        }

        auto file_sink = std::make_shared<sinks::basic_file_sink_mt>((directory / "venmic.log").string());

        file_sink->set_level(spdlog::level::trace);
        stdout_sink->set_level(spdlog::level::trace);

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
