#pragma once
#include "patchbay.hpp"

#include <string>

#include <cr/channel.hpp>
#include <rohrkabel/channel/channel.hpp>

namespace pw = pipewire;

namespace vencord
{
    struct list_nodes
    {
        std::vector<std::string> props;
    };

    struct set_target
    {
        std::vector<prop> include;
        std::vector<prop> exclude;
    };

    struct unset_target
    {
    };

    struct quit
    {
    };

    struct ready
    {
        bool success{true};
    };

    using pw_recipe = pw::recipe<list_nodes, set_target, unset_target, quit>;
    using cr_recipe = cr::recipe<std::vector<node>, ready>;
} // namespace vencord
