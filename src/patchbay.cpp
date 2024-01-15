#include "patchbay.impl.hpp"
#include "logger.hpp"

#include <future>
#include <optional>

#include <glaze/glaze.hpp>
#include <pulse/pulseaudio.h>

namespace vencord
{
    patchbay::~patchbay() = default;

    patchbay::patchbay() : m_impl(std::make_unique<impl>()) {}

    void patchbay::link(link_options options)
    {
        logger::get()->trace(R"([patchbay] (link) request: "{}")", glz::write_json(options));
        m_impl->sender->send(std::move(options));
    }

    void patchbay::unlink()
    {
        logger::get()->trace("[patchbay] unlink requested");
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

        if (!has_pipewire())
        {
            throw std::runtime_error("Not running PipeWire");
        }

        if (!instance)
        {
            instance = std::unique_ptr<patchbay>(new patchbay);
        }

        return *instance;
    }

    bool patchbay::has_pipewire()
    {
        static std::optional<bool> cached;

        if (cached)
        {
            logger::get()->trace("[patchbay] (has_pipewire) using cached result");
            return cached.value();
        }

        auto *loop    = pa_mainloop_new();
        auto *context = pa_context_new(pa_mainloop_get_api(loop), "venmic-pulse-info");

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
                logger::get()->error("[patchbay] (has_pipewire) failed to connect pulse context");
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
            logger::get()->error("[patchbay] (has_pipewire) result timed out");
            return false;
        }

        auto name = result.get();
        std::transform(name.begin(), name.end(), name.begin(), [](char c) { return std::tolower(c); });

        logger::get()->debug("[patchbay] (has_pipewire) pulse-server is \"{}\"", name);

        return cached.emplace(name.find("pipewire") != std::string::npos);
    }
} // namespace vencord
