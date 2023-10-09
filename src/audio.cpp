#include "audio.impl.hpp"

namespace vencord
{
    audio::~audio() = default;

    audio::audio() : m_impl(std::make_unique<impl>()) {}

    std::vector<node> audio::list()
    {
        m_impl->sender->send(list_nodes{});
        return m_impl->receiver->recv_as<std::vector<node>>();
    }

    void audio::link(std::string name)
    {
        m_impl->sender->send(set_target{std::move(name)});
    }

    audio &audio::get()
    {
        static std::unique_ptr<audio> instance;

        if (!instance)
        {
            instance = std::unique_ptr<audio>(new audio);
        }

        return *instance;
    }
} // namespace vencord