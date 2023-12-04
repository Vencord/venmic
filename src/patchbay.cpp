#include "patchbay.impl.hpp"
#include "logger.hpp"

#include <future>
#include <optional>
#include <pulse/pulseaudio.h>

namespace vencord
{
    patchbay::~patchbay() = default;

    patchbay::patchbay() : m_impl(std::make_unique<impl>()) {}

    void patchbay::link(std::vector<prop> include, std::vector<prop> exclude)
    {
        m_impl->sender->send(set_target{.include = std::move(include), .exclude = std::move(exclude)});
    }

    void patchbay::unlink()
    {
        m_impl->sender->send(unset_target{});
    }

    std::vector<node> patchbay::list(std::vector<std::string> props)
    {
        m_impl->sender->send(list_nodes{std::move(props)});
        return m_impl->receiver->recv_as<std::vector<node>>();
    }

    patchbay &patchbay::get()
    {
        static std::unique_ptr<patchbay> instance;

        if (instance)
        {
            return *instance;
        }

        if (!has_pipewire())
        {
            throw std::runtime_error("Not running PipeWire");
        }

        return *(instance = std::unique_ptr<patchbay>(new patchbay));
    }

    bool patchbay::has_pipewire()
    {
        static std::optional<bool> cached;

        if (cached)
        {
            logger::get()->debug("[has_pipewire] using cache");
            return cached.value();
        }

        auto *loop    = pa_mainloop_new();
        auto *context = pa_context_new(pa_mainloop_get_api(loop), "venmic-info");

        struct state
        {
            pa_mainloop *loop;
            std::promise<std::string> result;
        };

        static auto info = [](pa_context *, const pa_server_info *info, void *data)
        {
            auto &[loop, result] = *reinterpret_cast<state *>(data);
            result.set_value(info->server_name);
            pa_mainloop_quit(loop, 0);
        };

        static auto notify = [](pa_context *context, void *data)
        {
            auto &[loop, result] = *reinterpret_cast<struct state *>(data);
            auto state           = pa_context_get_state(context);

            if (state == PA_CONTEXT_FAILED)
            {
                logger::get()->error("failed to connect pulse context");
                pa_mainloop_quit(loop, 0);
                return;
            }

            if (state != PA_CONTEXT_READY)
            {
                return;
            }

            pa_context_get_server_info(context, info, data);
        };

        state state{loop};
        pa_context_set_state_callback(context, notify, &state);

        if (pa_context_connect(context, nullptr, PA_CONTEXT_NOFLAGS, nullptr) >= 0)
        {
            pa_mainloop_run(loop, nullptr);
        }

        pa_context_disconnect(context);
        pa_mainloop_free(loop);

        auto result = state.result.get_future();

        if (result.wait_for(std::chrono::seconds(0)) != std::future_status::ready)
        {
            logger::get()->error("[has_pipewire] result was not ready");
            return false;
        }

        auto name = result.get();
        std::transform(name.begin(), name.end(), name.begin(), [](char c) { return std::tolower(c); });

        logger::get()->debug("[has_pipewire] pulse-server was {}", name);

        auto rtn = name.find("pipewire") != std::string::npos;
        cached.emplace(rtn);

        return rtn;
    }
} // namespace vencord
