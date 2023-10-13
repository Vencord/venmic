#include "patchbay.impl.hpp"

namespace vencord
{
    patchbay::~patchbay() = default;

    patchbay::patchbay() : m_impl(std::make_unique<impl>()) {}

    std::set<std::string> patchbay::list()
    {
        m_impl->sender->send(list_nodes{});
        return m_impl->receiver->recv_as<std::set<std::string>>();
    }

    void patchbay::link(target target)
    {
        m_impl->sender->send(target);
    }

    void patchbay::unlink()
    {
        m_impl->sender->send(unset_target{});
    }

    patchbay &patchbay::get()
    {
        static std::unique_ptr<patchbay> instance;

        if (!instance)
        {
            instance = std::unique_ptr<patchbay>(new patchbay);
        }

        return *instance;
    }
} // namespace vencord
