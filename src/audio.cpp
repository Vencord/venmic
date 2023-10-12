#include "audio.impl.hpp"

namespace vencord
{
    audio::~audio() = default;

    audio::audio() : m_impl(std::make_unique<impl>()) {}

    std::set<std::string> audio::list()
    {
        m_impl->sender->send(list_nodes{});
        return m_impl->receiver->recv_as<std::set<std::string>>();
    }

    void audio::link(target target)
    {
        m_impl->sender->send(target);
    }

    void audio::unlink()
    {
        m_impl->sender->send(unset_target{});
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
